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

// An edit box that can live inside a list box, and that looks like a settings row
// rather than a bare text field.
//
// Two problems are solved here, both rooted in the same place: the game assumes an
// edit box is a direct child of a panel.
//
// FOCUS. CSWGuiEditbox::HandleFocusChange does the focus bookkeeping (keyboard mode
// on, GuiManager->focused_edit_box = this, caret visible) and then hands off to the
// parent *panel* via CSWGuiControl::HandleFocusChange, which calls
// `parent->AsSWGuiPanel()` and bails when that returns null. Our parent is a list
// box, whose AsSWGuiPanel is the game's shared "return NULL" stub, so the hand-off
// does nothing and the panel's active_control stays the list box.
// CSWGuiManager::HandleKeyPress then fails its
// `focused_edit_box == topModal->GetActiveControl()` check and typing goes nowhere.
// We do the hand-off ourselves, and refuse to be unfocused by anyone but the menu.
//
// APPEARANCE. CSWGuiEditbox::Draw paints a border and the editable text, and nothing
// else -- there is nowhere for the option's name and no hover feedback. We override
// Draw and compose it the way CSWGuiButtonToggle does: name text on the left, a
// bordered field on the right, and a highlight border swapped in on hover or focus.
class OptionsEditBox : public CSWGuiEditBox {
public:
    // Share of the row HEIGHT given to the name; the field takes the rest. Stacked
    // rather than side by side so the field gets the row's full width -- split
    // horizontally there was not enough room to type in.
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
        // Before anything else: _Draw reads the sub-objects we are about to free, and
        // the base destructor would not put the game's vtable back until after they
        // are gone.
        RestoreVTable();

        delete editText;
        delete border;
        delete hilight;
        delete nameText;
        delete nameParams;
    }

    // Builds the sub-objects and lays the row out. `hilightBorderParams` is the proto
    // item's highlight border; it is copied, not retained.
    void Initialize(CSWGuiExtent* rowExtent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* borderParams, CSWGuiBorderParams* hilightBorderParams,
                    const std::string& optionName)
    {
        // Lay the game's own parts out first, so GetBorder/GetEditText are valid.
        CSWGuiEditBox::Initialize(rowExtent, textParams, borderParams);

        border   = GetBorder();
        editText = GetEditText();

        // The name gets its own text object and its own params, so restyling it later
        // cannot disturb the editable text.
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

    // Splits the row: name on the left, bordered field on the right.
    //
    // Installed on the SetExtent virtual rather than done once here, because
    // CSWGuiListBox repositions its rows through this slot -- laying out at Initialize
    // alone would leave the name and field behind when the list scrolls.
    void _SetExtent(CSWGuiExtent* extent) {
        if (!extent) {
            return;
        }

        CSWGuiExtent nameExtent, fieldExtent;
        splitExtent(extent, &nameExtent, &fieldExtent);

        // The game's CSWGuiEditbox::SetExtent, which also moves the border and the
        // editable text. It writes the control extent too, hence the correction below.
        SetExtent(&fieldExtent);

        // The CONTROL keeps the whole row, so the name half still triggers hover and
        // the whole row stays hit-testable. This overload is a plain struct write with
        // no relayout -- not the game virtual we just called.
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

    // The game's version routes Enter and Escape through the parent panel, so both do
    // nothing for a box parented to a list box. Take them over: either one ends
    // editing, and dropping focus is what commits.
    void _HandleKeyPress(int key) {
        if (key == KEY_ESCAPE || key == KEY_RETURN || key == KEY_LINEFEED) {
            debugLog("[ModOptions]   EditBox(%p) key 0x%02X ends editing", GetPtr(), key);
            ReleaseFocus();
            return;
        }
        HandleKeyPress(key);
    }

    // Deliberate release, driven by the menu or by an end-editing key. Everything else
    // that asks us to give up focus is refused -- see _HandleFocusChange.
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
        // CSWGuiManager::HandleMouseMove unfocuses the panel's active control on every
        // mouse move that lands on a different control -- and it always does, because
        // the manager hit-checks down to the list box, never to a row inside it. Stock
        // menus never notice: their edit boxes are direct panel children, so the
        // control under the cursor IS the focused box. Ours can never be, so honouring
        // this would drop the caret the instant the mouse twitched.
        if (!hasFocus && !releasing) {
            // That runs per frame, so log the first refusal of a focus session only.
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

    // Hands the current text to OptionsMenu::commitOption. Defined out of line at the
    // bottom of OptionsMenu.h, where OptionsMenu is a complete type.
    void CommitToMenu();

    // The panel that owns the list box we sit in. Not owned.
    CSWGuiPanel* owner;

private:
    // Same object as `owner`, typed for the commit callback. Incomplete here, so it is
    // only ever dereferenced from CommitToMenu.
    OptionsMenu* menu;

    // Sub-objects we allocate and free. The game's CSWGuiEditbox struct has no room
    // for either, so they live beside it and are drawn by _Draw.
    CSWGuiText* nameText = nullptr;
    CSWGuiTextParams* nameParams = nullptr;
    CSWGuiBorder* hilight = nullptr;

    // Cached wrappers over the game object's own parts. GetBorder/GetEditText heap
    // allocate on every call, and _Draw runs every frame.
    CSWGuiBorder* border = nullptr;
    CSWGuiEditText* editText = nullptr;

    // Name on top, field below. Stacked rather than side by side so the field keeps
    // the row's full width.
    void splitExtent(CSWGuiExtent* row, CSWGuiExtent* outName, CSWGuiExtent* outField) {
        const int nameHeight = row->height * NAME_HEIGHT_PERCENT / 100;
        *outName  = { row->left, row->top, row->width, nameHeight };
        *outField = { row->left, row->top + nameHeight, row->width, row->height - nameHeight };
    }

    bool focused = false;

    // True only inside ReleaseFocus, so _HandleFocusChange can tell our own release
    // apart from the game's.
    bool releasing = false;

    // Keeps the per-frame refusal out of the log after the first one.
    bool loggedRefusal = false;

    // Keys CSWGuiEditbox::HandleKeyPress recognises but cannot act on for us.
    static const int KEY_LINEFEED = 0x0A;
    static const int KEY_RETURN   = 0x0D;
    static const int KEY_ESCAPE   = 0x1B;
};
