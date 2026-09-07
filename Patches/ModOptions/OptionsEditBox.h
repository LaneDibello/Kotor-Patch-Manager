#pragma once
#include "Common.h"
#include "GameAPI/CResGFF.h"
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
    static const int NAME_HEIGHT_PERCENT = 40;

    explicit OptionsEditBox(CSWGuiPanel* owner, OptionsMenu* menu)
        : CSWGuiEditBox(), owner(owner), menu(menu)
    {
        // Built here, not in Configure: Load runs before anything else and needs
        // somewhere to put the name text and the HILIGHT block.
        nameText = new CSWGuiText();
        hilight  = new CSWGuiBorder();

        OverrideLoad(memberFuncAddr(&OptionsEditBox::_Load));
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
        // nameParams wraps memory inside nameText, so it goes first.
        delete nameParams;
        delete nameText;
    }


    // CSWGuiEditbox::Load reads TEXT and BORDER but has no notion of a focus frame,
    // so it ignores HILIGHT. Pick that up, and the name line's params with it.
    void _Load(CResGFF* gff, CResStruct* item) {
        CSWGuiEditBox::LoadFromGff(gff, item);

        border   = GetBorder();
        editText = GetEditText();

        if (nameText) {
            CExoString textLabel(const_cast<char*>("TEXT"));
            nameText->Load(gff, item, &textLabel);
            // Load copies the block into the text's own embedded params; that copy
            // is what Draw reads, so it is what the name has to be written to.
            delete nameParams;
            nameParams = nameText->GetTextParams();
        }

        if (hilight) {
            CExoString hilightLabel(const_cast<char*>("HILIGHT"));
            hilight->Load(gff, item, &hilightLabel);
        }
    }

    // Applies the option to a control the layout has already styled and sized.
    void Configure(int rowWidth, const std::string& optionName, const std::string& value) {
        if (nameParams) {
            CExoString name(const_cast<char*>(optionName.c_str()));
            nameParams->SetText(&name);
        }

        // Height and art came from the layout; only the width is ours, because rows
        // stretch to the list box viewport. Position is the list box's business.
        CSWGuiExtent row = GetExtent();
        row.left = 0;
        row.top = 0;
        row.width = rowWidth;
        _SetExtent(&row);

        if (editText) {
            CExoString textValue(const_cast<char*>(value.c_str()));
            editText->SetText(&textValue);
        }
    }

    void _SetExtent(CSWGuiExtent* extent) {
        if (!extent) {
            return;
        }

        // Copy before anything else: the base SetExtent writes control.extent, and
        // a caller is free to pass a pointer to that very field.
        CSWGuiExtent row = *extent;

        CSWGuiExtent nameExtent, fieldExtent;
        splitExtent(&row, &nameExtent, &fieldExtent);
        SetExtent(&fieldExtent);
        CSWGuiObject::SetExtent(row);

        // Move, do not re-Load: the art is already baked in and re-running Load
        // would re-read the layout for no reason.
        if (nameText) {
            nameText->SetExtent(&nameExtent);
        }
        if (hilight) {
            hilight->CSWGuiObject::SetExtent(fieldExtent);
        }
    }

    // Replaces CSWGuiEditbox::Draw, which paints no hilight. Focus counts as well as
    // hover, so a sticky-focused field stays marked once the mouse leaves.
    // Runs every frame: wrappers are cached members, never fetched here.
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
            // Through the menu, not ReleaseFocus directly: dropping focus also has
            // to hand the panel's active control back, or the arrow keys keep
            // arriving here and the list box stops navigating.
            ReleaseToMenu();
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
        // CSWGuiManager::HandleMouseMove unfocuses the panel's active control on every
        // mouse move landing elsewhere, and it always does: the manager hit-checks down
        // to the list box, never to a row inside it. Only the menu releases focus.
        if (!hasFocus && !releasing) {
            return;
        }

        HandleFocusChange(hasFocus);
        focused = (hasFocus != 0);

        if (hasFocus && owner) {
            // Safe with a control the panel does not own: SetActiveControl only swaps
            // active_control and fires HoverExit/HoverEnter, with no membership check
            // and no second focus change to recurse through.
            owner->SetActiveControl(this, 0);
        }
    }

    bool IsFocused() const { return focused; }

    void CommitToMenu();
    void ReleaseToMenu();

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

    // Keys CSWGuiEditbox::HandleKeyPress recognises but cannot act on for us.
    static const int KEY_LINEFEED = 0x0A;
    static const int KEY_RETURN   = 0x0D;
    static const int KEY_ESCAPE   = 0x1B;
};
