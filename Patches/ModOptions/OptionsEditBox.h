#pragma once
#include "Common.h"
#include "GameAPI/CSWGuiEditBox.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiPanel.h"

// An editbox that can live inside a list box.
//
// CSWGuiEditbox::HandleFocusChange does two things: the focus bookkeeping (keyboard
// mode on, GuiManager->focused_edit_box = this, caret visible), and then a hand-off
// to the control's parent *panel* via CSWGuiControl::HandleFocusChange, which does
// `parent->AsSWGuiPanel()` and bails when that returns null.
//
// Our parent is a CSWGuiListBox, and a control's AsSWGuiPanel is the game's shared
// "return NULL" stub, so the hand-off silently does nothing and the panel's
// active_control stays the list box. CSWGuiManager::HandleKeyPress then fails its
// `focused_edit_box == topModal->GetActiveControl()` check and never reaches
// CSWGuiEditbox::HandleKeyPress -- typing does nothing.
//
// So we override the virtual, let the game do the bookkeeping half, and perform the
// hand-off ourselves against the panel we were told about.
class OptionsEditBox : public CSWGuiEditBox {
public:
    explicit OptionsEditBox(CSWGuiPanel* owner)
        : CSWGuiEditBox(), owner(owner)
    {
        OverrideHandleFocusChange(memberFuncAddr(&OptionsEditBox::_HandleFocusChange));
    }

    // Deliberate release, driven by the menu. Everything else that asks us to give
    // up focus is refused -- see _HandleFocusChange.
    void ReleaseFocus() {
        if (!focused) {
            return;
        }
        releasing = true;
        _HandleFocusChange(0);
        releasing = false;
    }

    void _HandleFocusChange(int hasFocus) {
        // CSWGuiManager::HandleMouseMove unfocuses the edit box on every mouse move
        // that lands on a different control -- and it always does, because the manager
        // hit-checks down to the list box, never to a row inside it. Stock menus never
        // notice: their edit boxes are direct panel children, so the control under the
        // cursor IS the focused box. Ours can never be, so honouring this would drop
        // the caret the instant the mouse twitched. Only the menu releases focus.
        if (!hasFocus && !releasing) {
            // CSWGuiManager::HandleMouseMove runs this on every mouse-move frame, so
            // log the first refusal of each focus session and stay quiet after that.
            if (focused && !loggedRefusal) {
                loggedRefusal = true;
                debugLog("[ModOptions]   EditBox(%p) refusing unfocus from %p (sticky; silenced)",
                         GetPtr(), LastFocusChangeCaller());
            }
            return;
        }

        void* before = owner ? panelActiveControl() : nullptr;
        HandleFocusChange(hasFocus);
        focused = (hasFocus != 0);
        loggedRefusal = false;

        debugLog("[ModOptions]   EditBox(%p)::HandleFocusChange(%i) from %p, panel active %p -> %p",
                 GetPtr(), hasFocus, LastFocusChangeCaller(), before,
                 owner ? panelActiveControl() : nullptr);
        probeManagerForSelf(hasFocus);

        if (hasFocus && owner) {
            // Safe with a control the panel does not own: SetActiveControl only
            // swaps active_control and fires HoverExit/HoverEnter, with no
            // membership check and no second focus change to recurse through.
            owner->SetActiveControl(this, 0);
            debugLog("[ModOptions]   EditBox(%p) pinned panel active_control -> %p",
                     GetPtr(), panelActiveControl());
        }
    }

    bool IsFocused() const { return focused; }

    // The panel that owns the list box we sit in. Not owned.
    CSWGuiPanel* owner;

private:
    void* panelActiveControl() {
        if (!owner) {
            return nullptr;
        }
        CSWGuiControl* active = owner->GetActiveControl();
        void* ptr = active ? active->GetPtr() : nullptr;
        delete active;
        return ptr;
    }

    // DEBUG: CSWGuiManager::focused_edit_box has no offset row in the address DB, so
    // find it by observation -- sweep the manager (168 bytes) for a field holding our
    // game pointer right after the game set it, and for that field going null again
    // on release. Logs the offset so it can go into the DB, after which this and
    // panelActiveControl() can be replaced by real accessors.
    void probeManagerForSelf(int hasFocus) {
        CSWGuiManager manager;
        char* base = static_cast<char*>(manager.GetPtr());
        if (!base || !GetPtr()) {
            return;
        }

        for (int offset = 0; offset + 4 <= 0xA8; offset += 4) {
            void* field = *reinterpret_cast<void**>(base + offset);
            if (hasFocus && field == GetPtr()) {
                debugLog("[ModOptions]   manager+0x%02X == this editbox (focused_edit_box?)", offset);
            }
        }
    }

    // Mirrors GuiManager->focused_edit_box for this box. The game only clears that
    // from HandleFocusChange(0), and nothing calls it when focus moves to another
    // control parented to the same list box -- so the menu drives the release and
    // needs to know which box to release. See OptionsMenu::releaseKeyboardFocus.
    bool focused = false;

    // True only inside ReleaseFocus, so _HandleFocusChange can tell our own release
    // apart from the game's.
    bool releasing = false;

    // Keeps the per-frame refusal out of the log after the first one.
    bool loggedRefusal = false;
};
