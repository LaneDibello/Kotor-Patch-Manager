#pragma once
#include "Common.h"
#include "GameAPI/CSWGuiListBox.h"

class OptionsMenu;
class OptionsSlider;
class OptionsListSelect;

// A list box keeps the mouse capture and only fires AButton, which leaves a slider
// inert. Its own mouse handlers are no use either -- they resolve coordinates as if
// the row sat on a panel -- so the press and drag are driven from here.
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

	// Mouse position in the space row extents live in: panel-local, less the
	// viewport origin. This is what CSWGuiListBox::HitCheckMouseLocal computes
	// before handing coordinates to a row. Public because a row needs it too --
	// nothing else tracks the mouse over a sub-control.
	bool LocalContentCoords(int* outX, int* outY);

private:
	OptionsMenu* menu = nullptr;

	// The slider whose thumb the mouse is holding, if any.
	OptionsSlider* dragging = nullptr;
};
