#pragma once
#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiImage.h"
#include "GameAPI/CSWGuiSlider.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"

#include <string>

class OptionsMenu;

// A custom control class that derives from CSWGuiSlider.
// The game's slider is border + hilight + thumb and nothing else, so this adds the
// option's name and its current value on a line above the track.
//
// It also carries the option's `min`. The game's slider has no minimum -- cur_value
// runs 0..max_value -- so the TOML minimum is an offset this class applies on the
// way in and out, and nothing below this class ever sees it.
class OptionsSlider : public CSWGuiSlider {
public:
	// Layout const, this is the percentage of the extent the `name` takes up
	static const int NAME_HEIGHT_PERCENT = 40;

	explicit OptionsSlider(OptionsMenu* menu)
		: CSWGuiSlider(), menu(menu)
	{
		OverrideDraw(memberFuncAddr(&OptionsSlider::_Draw));
		OverrideSetExtent(memberFuncAddr(&OptionsSlider::_SetExtent));
	}

	~OptionsSlider() {
		RestoreVTable();

		delete nameText;
		delete nameParams;
	}

	void Initialize(CSWGuiExtent* rowExtent, CSWGuiTextParams* textParams,
	                CSWGuiBorderParams* borderParams, CSWGuiBorderParams* hilightParams,
	                const std::string& optionName, int min, int max, int value)
	{
		optionMin = min;
		name = optionName;

		CSWGuiExtent nameExtent;
		splitExtent(rowExtent, &nameExtent, &trackExtent);

		// Lay the game's own parts out first, so the border/image members are valid.
		CResRef thumb("lbl_optslidera");
		CSWGuiSlider::Initialize(&trackExtent, borderParams, hilightParams, &thumb);

		// Range before value: CSWGuiSlider::SetExtent divides by max_value to place
		// the thumb, so a max of 0 on the first layout puts it in the wrong place.
		SetMaxValue(max - min);
		SetStoredValue(value);

		nameText = new CSWGuiText();
		nameParams = new CSWGuiTextParams();
		if (nameParams && textParams) {
			*nameParams = *textParams;
		}

		// Bake the sub-object ONCE, here, while the caller still has the border art
		// borrowed onto the proto item's params -- same reasoning as OptionsEditBox.
		nameText->Initialize(&nameExtent, nameParams, 1.0f);

		RefreshLabel();
		_SetExtent(rowExtent);

		debugLog("[ModOptions] slider `%s` range %i..%i value %i -> max_value %i cur_value %i",
			name.c_str(), min, max, value, GetMaxValue(), GetCurValue());
	}

	// The value as the option means it. The control only ever holds value - min.
	int StoredValue() { return optionMin + GetCurValue(); }

	// SetCurValue clamps into [0, max_value] itself, so a hand-edited ini that is
	// out of range lands on the nearest end rather than off the track.
	void SetStoredValue(int value) { SetCurValue(value - optionMin); }

	// Rewrites the label to match the thumb. Touches text params only, so it is
	// cheap enough to call on every frame of a drag.
	void RefreshLabel() {
		if (!nameParams) {
			return;
		}
		std::string label = name + "   " + std::to_string(StoredValue());
		CExoString text(const_cast<char*>(label.c_str()));
		nameParams->SetText(&text);
	}

	// SetCurValue and SetMaxValue both end in a tail call to the virtual SetExtent,
	// passing &this->control.extent -- so the argument ALIASES the field the base
	// SetExtent is about to write. Copy it before anything else runs, or the split
	// track extent gets read back as the row extent and the row loses 40% of its
	// height on every value change.
	void _SetExtent(CSWGuiExtent* extent) {
		if (!extent) {
			return;
		}
		CSWGuiExtent row = *extent;

		CSWGuiExtent nameExtent;
		splitExtent(&row, &nameExtent, &trackExtent);

		// The base lays the border and thumb out against the track rect, then the
		// row rect goes back so the list box still sees a full-height row. Left and
		// width are identical in both, so the drag math is unaffected.
		SetExtent(&trackExtent);
		CSWGuiObject::SetExtent(row);

		// Move, do not re-Initialize: the art is already baked in.
		if (nameText) {
			nameText->SetExtent(&nameExtent);
		}
	}

	// CSWGuiSlider::Draw already swaps border for hilight on bit flag 1 and paints
	// the thumb; all this adds is the name line.
	//
	// Runs every frame: the wrappers are cached members, never fetched here.
	void _Draw(float alpha) {
		CSWGuiSlider::Draw(alpha);
		if (nameText) {
			nameText->Draw(alpha);
		}
	}

	// Places the thumb from a mouse x in the space the row extent is in. Mirrors
	// what CSWGuiSlider::HandleMouseCapturedMovement does, which we cannot call:
	// it recomputes the position itself, through a gui object that for a list box
	// row resolves to the wrong space.
	void SetValueFromTrackX(int x) {
		if (trackExtent.width <= 0) {
			return;
		}
		int offset = x - trackExtent.left;
		if (offset < 0) { offset = 0; }
		if (offset > trackExtent.width) { offset = trackExtent.width; }

		SetCurValue(GetMaxValue() * offset / trackExtent.width);
		RefreshLabel();
	}

	// 1 thumb, 2 track before it, 3 track after it, 0 miss.
	int HitTest(int x, int y) { return HitCheckSlider(x, y); }

	void CommitToMenu();

private:
	OptionsMenu* menu;

	std::string name;
	int optionMin = 0;
	CSWGuiExtent trackExtent{};

	CSWGuiText* nameText = nullptr;
	CSWGuiTextParams* nameParams = nullptr;

	void splitExtent(CSWGuiExtent* row, CSWGuiExtent* outName, CSWGuiExtent* outTrack) {
		const int nameHeight = row->height * NAME_HEIGHT_PERCENT / 100;
		*outName  = { row->left, row->top, row->width, nameHeight };
		*outTrack = { row->left, row->top + nameHeight, row->width, row->height - nameHeight };
	}
};
