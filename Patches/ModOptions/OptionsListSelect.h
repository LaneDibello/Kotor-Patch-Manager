#pragma once
#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"
#include "ModOptionsConfig.h"
#include "OptionsLayout.h"

#include <string>
#include <vector>

class OptionsMenu;

// Derives from CSWGuiButton to show one of an option's `choices`, with a left and a
// right button either side of it. The vanilla equivalent -- the anisotropy control on
// optgraphicsadv -- is three separate panel controls, but a list box row has to be a
// single control, so the two arrows are owned, laid out, drawn and hit-tested here.
class OptionsListSelect : public CSWGuiButton {
public:
	explicit OptionsListSelect(OptionsMenu* menu)
		: CSWGuiButton(), menu(menu)
	{
		// Built here, not in LoadChildren: the arrows are what the row is, so a
		// failed load is the only thing that should leave them empty.
		leftButton = new CSWGuiButton();
		rightButton = new CSWGuiButton();

		OverrideDraw(memberFuncAddr(&OptionsListSelect::_Draw));
		OverrideSetExtent(memberFuncAddr(&OptionsListSelect::_SetExtent));
	}

	~OptionsListSelect() {
		RestoreVTable();

		// valueParams wraps memory inside the button's own text, so it goes first.
		delete valueParams;
		delete valueText;

		// Never handed to the game, so these are ours to free outright.
		delete leftButton;
		delete rightButton;
	}

	// The arrows come from their own CONTROLS entries, so they arrive fully styled.
	// CSWGuiButton::Load reads BORDER, HILIGHT and TEXT, which is all any of the
	// three templates carries -- hence no OverrideLoad on this class.
	bool LoadChildren(OptionsLayout& layout, CSWGuiObject* owner) {
		if (!leftButton || !rightButton) {
			return false;
		}
		if (!layout.Load(leftButton, owner, "OPT_LIST_LEFT")) {
			return false;
		}
		if (!layout.Load(rightButton, owner, "OPT_LIST_RIGHT")) {
			return false;
		}

		// Only their size is taken from the layout; where they sit is the row's call.
		arrowWidth = leftButton->GetExtent().width;
		arrowHeight = leftButton->GetExtent().height;
		return true;
	}

	// Applies the option to a control the layout has already styled and sized.
	void Configure(int rowWidth, const ModOption& option, const std::string& value) {
		choices = option.choices;
		index = 0;

		bool matched = false;
		for (size_t choice = 0; choice < choices.size(); ++choice) {
			if (choices[choice] == value) {
				index = choice;
				matched = true;
				break;
			}
		}
		if (!matched) {
			// A hand-edited ini can hold anything; the default is the only sane landing.
			index = option.DefaultChoiceIndex();
			if (index >= choices.size()) {
				index = 0;
			}
			debugLog("[ModOptions] `%s` holds `%s`, which is not one of its choices; showing `%s`",
				option.name.c_str(), value.c_str(), choices.empty() ? "" : choices[index].c_str());
		}

		// The text params live inside the button's own text block, which Load has
		// already filled in. Cached because RefreshText runs on every step.
		delete valueParams;
		delete valueText;
		valueText = GetText();
		valueParams = valueText ? valueText->GetTextParams() : nullptr;

		// Height and art came from the layout; only the width is ours, because rows
		// stretch to the list box viewport. Position is the list box's business.
		CSWGuiExtent row = GetExtent();
		row.left = 0;
		row.top = 0;
		row.width = rowWidth;
		_SetExtent(&row);

		RefreshText();
	}

	// The list box calls this through the vtable when it positions the row, so the
	// arrows follow it. Copy first: the base SetExtent writes control.extent, and a
	// caller is free to pass a pointer to that very field.
	void _SetExtent(CSWGuiExtent* extent) {
		if (!extent) {
			return;
		}
		CSWGuiExtent row = *extent;

		const int width = (arrowWidth < row.width / 2) ? arrowWidth : row.width / 2;
		const int height = (arrowHeight < row.height) ? arrowHeight : row.height;
		const int top = row.top + (row.height - height) / 2;

		leftExtent = { row.left, top, width, height };
		rightExtent = { row.left + row.width - width, top, width, height };

		// The base lays its border and its text out between the arrows, then the row
		// rect goes back so the list box still sees a full-size row.
		CSWGuiExtent inner = { row.left + width, row.top, row.width - 2 * width, row.height };
		CSWGuiButton::SetExtent(&inner);
		CSWGuiObject::SetExtent(row);

		// Move, do not re-Load: the art is already baked in.
		if (leftButton) {
			leftButton->SetExtent(&leftExtent);
		}
		if (rightButton) {
			rightButton->SetExtent(&rightExtent);
		}
	}

	// CSWGuiButton::Draw handles this control's own border and text; the arrows are
	// not in anything that would draw them, so they are drawn from here.
	// Runs every frame: wrappers are cached members, never fetched here.
	void _Draw(float alpha) {
		CSWGuiButton::Draw(alpha);

		// Bit flag 0 is the hover flag CSWGuiButton::Draw reads to pick HILIGHT over
		// BORDER. The list box only ever sets it on the row, never on a sub-control.
		int x = 0, y = 0;
		const int hovered = RowMouseCoords(&x, &y) ? HitTest(x, y) : 0;

		if (leftButton) {
			leftButton->SetControlBitFlag(0, hovered == HIT_LEFT);
			leftButton->Draw(alpha);
		}
		if (rightButton) {
			rightButton->SetControlBitFlag(0, hovered == HIT_RIGHT);
			rightButton->Draw(alpha);
		}
	}

	static const int HIT_NONE = 0;
	static const int HIT_LEFT = 1;
	static const int HIT_RIGHT = 2;

	// Coordinates arrive in the space row extents live in -- panel-local, less the
	// viewport origin -- which is what OptionsListBox::LocalContentCoords produces.
	int HitTest(int x, int y) {
		if (Contains(leftExtent, x, y)) {
			return HIT_LEFT;
		}
		if (Contains(rightExtent, x, y)) {
			return HIT_RIGHT;
		}
		return HIT_NONE;
	}

	// Walks the choices, wrapping at both ends, and writes the new value through.
	void Step(int delta) {
		if (choices.size() < 2) {
			return;
		}

		const int count = (int)choices.size();
		int next = ((int)index + delta) % count;
		if (next < 0) {
			next += count;
		}
		index = (size_t)next;

		RefreshText();
		PlayStepSound();
		CommitToMenu();
	}

	void RefreshText() {
		if (!valueParams || index >= choices.size()) {
			return;
		}
		CExoString text(const_cast<char*>(choices[index].c_str()));
		valueParams->SetText(&text);
	}

	// The choice the control is showing, in the form an ini entry wants.
	const std::string& Value() const {
		static const std::string empty;
		return (index < choices.size()) ? choices[index] : empty;
	}

	void CommitToMenu();

private:
	OptionsMenu* menu;

	std::vector<std::string> choices;
	size_t index = 0;

	CSWGuiButton* leftButton = nullptr;
	CSWGuiButton* rightButton = nullptr;

	// Template size, and where the arrows ended up in the row.
	int arrowWidth = 0;
	int arrowHeight = 0;
	CSWGuiExtent leftExtent{};
	CSWGuiExtent rightExtent{};

	CSWGuiText* valueText = nullptr;
	CSWGuiTextParams* valueParams = nullptr;

	static bool Contains(const CSWGuiExtent& extent, int x, int y) {
		return extent.width > 0 && extent.height > 0
			&& x >= extent.left && x < extent.left + extent.width
			&& y >= extent.top && y < extent.top + extent.height;
	}

	// Both defined with the rest of the menu's inline members, which is the only
	// place OptionsMenu is a complete type.
	bool RowMouseCoords(int* outX, int* outY);
	void PlayStepSound();
};
