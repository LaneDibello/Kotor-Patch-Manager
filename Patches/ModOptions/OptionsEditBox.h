#pragma once
#include "Common.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiEditBox.h"
#include "GameAPI/CSWGuiEditText.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"

#include <string>

class OptionsMenu;

// A custom control class that Derives from CSWGuiEditBox
// Has some fancy custom layout behavior
// Includes a name and hilight border
// Is also able to play nice with A CSWGuiListBox as its parent
class OptionsEditBox : public CSWGuiEditBox {
public:
    // Layout const, this is the percentage of the extent the `name` takes up
    static const int NAME_HEIGHT_PERCENT = 30;

    explicit OptionsEditBox(CSWGuiPanel* owner, OptionsMenu* menu)
        : CSWGuiEditBox(), owner(owner), menu(menu)
    {
        OverrideHandleFocusChange(memberFuncAddr(&OptionsEditBox::_HandleFocusChange));
        OverrideHandleKeyPress(memberFuncAddr(&OptionsEditBox::_HandleKeyPress));
        OverrideSetExtent(memberFuncAddr(&OptionsEditBox::_SetExtent));
        OverrideDraw(memberFuncAddr(&OptionsEditBox::_Draw));
    }

    ~OptionsEditBox() {
        RestoreVTable();

        delete editText;
        delete border;
        delete hilight;
        delete nameText;
        delete nameParams;
    }


    void Initialize(CSWGuiExtent* rowExtent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* borderParams, CSWGuiBorderParams* hilightBorderParams,
                    const std::string& optionName)
    {
        // Lay the game's own parts out first, so GetBorder/GetEditText are valid.
        CSWGuiEditBox::Initialize(rowExtent, textParams, borderParams);

        border   = GetBorder();
        editText = GetEditText();

        nameText   = new CSWGuiText();
        nameParams = new CSWGuiTextParams();
        if (nameParams && textParams) {
            *nameParams = *textParams;
            CExoString name(const_cast<char*>(optionName.c_str()));
            nameParams->SetText(&name);
        }

        hilight = new CSWGuiBorder();

        // Bake the sub-objects ONCE, here, while the caller still has the border art
        // borrowed onto the proto item's params. CSWGuiBorder::Initialize copies those
        // params into the border, so from here the highlight owns its own art -- if a
        // relayout re-ran Initialize it would re-read the proto's params, which the
        // caller has by then put back, and the toggle's checkbox fill would return.
        CSWGuiExtent nameExtent, fieldExtent;
        splitExtent(rowExtent, &nameExtent, &fieldExtent);
        nameText->Initialize(&nameExtent, nameParams, 1.0f);
        hilight->Initialize(&fieldExtent, hilightBorderParams);

        _SetExtent(rowExtent);
    }

    void _SetExtent(CSWGuiExtent* extent) {
        if (!extent) {
            return;
        }

        CSWGuiExtent nameExtent, fieldExtent;
        splitExtent(extent, &nameExtent, &fieldExtent);
        SetExtent(&fieldExtent);
        CSWGuiObject::SetExtent(*extent);

        // Move, do not re-Initialize: the art is already baked in and re-running
        // Initialize would re-read params that no longer hold our images.
        if (nameText) {
            nameText->SetExtent(&nameExtent);
        }
        if (hilight) {
            hilight->CSWGuiObject::SetExtent(fieldExtent);
        }
    }

    // Replaces CSWGuiEditbox::Draw, which only paints border + text. Shaped like
    // CSWGuiButton::Draw: swap the border art on bit flag 1 (hover), then paint the
    // pieces. Focus is included so a sticky-focused field stays marked once the
    // mouse moves away.
    //
    // Runs every frame: the wrappers are cached members, never fetched here.
    void _Draw(float alpha) {
        CSWGuiBorder* frame = (GetControlBitFlag(0) || focused) ? hilight : border;
        if (frame) {
            frame->Draw(alpha);
        }
        if (nameText) {
            nameText->Draw(alpha);
        }
        if (editText) {
            editText->Draw(alpha);
        }
    }

    void _HandleKeyPress(int key) {
        if (key == KEY_ESCAPE || key == KEY_RETURN || key == KEY_LINEFEED) {
            debugLog("[ModOptions]   EditBox(%p) key 0x%02X ends editing", GetPtr(), key);
            ReleaseFocus();
            return;
        }
        HandleKeyPress(key);
    }

    void ReleaseFocus() {
        if (!focused) {
            return;
        }
        releasing = true;
        _HandleFocusChange(0);
        releasing = false;

        // Focus loss is the commit point, for every path that reaches it.
        CommitToMenu();
    }

    void _HandleFocusChange(int hasFocus) {
        if (!hasFocus && !releasing) {
            if (focused && !loggedRefusal) {
                loggedRefusal = true;
                debugLog("[ModOptions]   EditBox(%p) refusing unfocus from %p (sticky; silenced)",
                         GetPtr(), LastFocusChangeCaller());
            }
            return;
        }

        HandleFocusChange(hasFocus);
        focused = (hasFocus != 0);
        loggedRefusal = false;

        // LastFocusChangeCaller is only written by the vtable thunk, so it is stale on
        // a release we drove ourselves. Say so rather than print a misleading address.
        if (releasing) {
            debugLog("[ModOptions]   EditBox(%p)::HandleFocusChange(0) (menu-driven)", GetPtr());
        } else {
            debugLog("[ModOptions]   EditBox(%p)::HandleFocusChange(%i) from %p",
                     GetPtr(), hasFocus, LastFocusChangeCaller());
        }

        if (hasFocus && owner) {
            // Safe with a control the panel does not own: SetActiveControl only swaps
            // active_control and fires HoverExit/HoverEnter, with no membership check
            // and no second focus change to recurse through.
            owner->SetActiveControl(this, 0);
        }
    }

    bool IsFocused() const { return focused; }

    void CommitToMenu();

    CSWGuiPanel* owner;

private:
    OptionsMenu* menu;

    CSWGuiText* nameText = nullptr;
    CSWGuiTextParams* nameParams = nullptr;
    CSWGuiBorder* hilight = nullptr;

    CSWGuiBorder* border = nullptr;
    CSWGuiEditText* editText = nullptr;

    void splitExtent(CSWGuiExtent* row, CSWGuiExtent* outName, CSWGuiExtent* outField) {
        const int nameHeight = row->height * NAME_HEIGHT_PERCENT / 100;
        *outName  = { row->left, row->top, row->width, nameHeight };
        *outField = { row->left, row->top + nameHeight, row->width, row->height - nameHeight };
    }

    bool focused = false;
    bool releasing = false;
    bool loggedRefusal = false;

    // Keys CSWGuiEditbox::HandleKeyPress recognises but cannot act on for us.
    static const int KEY_LINEFEED = 0x0A;
    static const int KEY_RETURN   = 0x0D;
    static const int KEY_ESCAPE   = 0x1B;
};
