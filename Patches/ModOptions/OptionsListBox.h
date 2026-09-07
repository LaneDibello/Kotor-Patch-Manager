#pragma once
#include "Common.h"
#include "GameAPI/CSWGuiListBox.h"

class OptionsMenu;
class OptionsSlider;

// CSWGuiListBox::HandleLMouseDown never forwards the click to the row it hit: it does
// its own selection bookkeeping, then takes the mouse capture for itself, and its
// HandleMouseCapturedMovement only ever drives its scrollbar. A toggle survives that
// because HandleLMouseUp fires AButton, which is all a toggle needs; a slider needs
// the press position and every mouse move after it.
//
// The slider's own mouse entry points cannot be reused: they resolve coordinates
// through the control's gui object as a panel, which for a row is this list box, and
// row extents are in content space anyway (HitCheckMouseLocal subtracts the viewport
// origin before it hit-tests them). So the press is hit-tested here, in the space the
// row extents are actually in, and the drag is driven from here too.
class OptionsListBox : public CSWGuiListBox {
public:
	~OptionsListBox() { RestoreVTable(); }

	// Call once the layout has populated the control.
	void HookMouse(OptionsMenu* owner) {
		menu = owner;
		OverrideHandleLMouseDown(memberFuncAddr(&OptionsListBox::_HandleLMouseDown));
		OverrideHandleMouseCapturedMovement(memberFuncAddr(&OptionsListBox::_HandleMouseCapturedMovement));
		OverrideHandleLMouseUp(memberFuncAddr(&OptionsListBox::_HandleLMouseUp));
	}

	void _HandleLMouseDown();
	int _HandleMouseCapturedMovement(int x, int y);
	void _HandleLMouseUp();

private:
	OptionsMenu* menu = nullptr;

	// The slider whose thumb the mouse is holding, if any.
	OptionsSlider* dragging = nullptr;

	// Mouse position in the space row extents live in: panel-local, less the
	// viewport origin. This is what CSWGuiListBox::HitCheckMouseLocal computes
	// before handing coordinates to a row.
	bool LocalContentCoords(int* outX, int* outY);
};
