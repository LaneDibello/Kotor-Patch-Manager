#pragma once
#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResGFF.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiSlider.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"
#include "ModOptionsConfig.h"

#include <string>

class OptionsMenu;

// Derives from CSWGuiSlider, which is border + hilight + thumb and nothing else, to
// add the option's name and value on a line above the track. Also holds the option's
// `min`, applied as an offset here so nothing below ever sees it.
class OptionsSlider : public CSWGuiSlider {
public:
	// The track matches the game's own gamma slider at 24. The rest of the row --
	// its total height comes from OPT_SLIDER's EXTENT in the layout -- goes to the
	// name line, less a small gap so the track does not sit on the option below.
	static const int TRACK_HEIGHT = 24;
	static const int BOTTOM_MARGIN_PERCENT = 5;

	explicit OptionsSlider(OptionsMenu* menu)
		: CSWGuiSlider(), menu(menu)
	{
		// Built here, not in Configure: Load runs before anything else and needs
		// somewhere to put the TEXT block.
		nameText = new CSWGuiText();

		OverrideLoad(memberFuncAddr(&OptionsSlider::_Load));
		OverrideDraw(memberFuncAddr(&OptionsSlider::_Draw));
		OverrideSetExtent(memberFuncAddr(&OptionsSlider::_SetExtent));
	}

	~OptionsSlider() {
		RestoreVTable();

		// nameParams wraps memory inside nameText, so it goes first.
		delete nameParams;
		delete nameText;
	}

	// CSWGuiSlider::Load reads BORDER, HILIGHT, THUMB and MAXVALUE, but a slider has
	// no text of its own, so it ignores TEXT. Pick that up for the name line.
	void _Load(CResGFF* gff, CResStruct* item) {
		CSWGuiSlider::LoadFromGff(gff, item);

		if (nameText) {
			CExoString label(const_cast<char*>("TEXT"));
			nameText->Load(gff, item, &label);

			// Load copies the block into the text's own embedded params. That copy is
			// what Draw reads, so it is also what RefreshLabel has to write to.
			delete nameParams;
			nameParams = nameText->GetTextParams();
		}
	}

	// Applies the option to a control the layout has already styled and sized.
	void Configure(int rowWidth, const ModOption& option, int value) {
		optionMin = option.min;
		name = option.name;

		// Range before value: CSWGuiSlider::SetExtent divides by max_value to place
		// the thumb, so a max of 0 on the first layout puts it in the wrong place.
		SetMaxValue(option.max - option.min);

		// Height and art came from the layout; only the width is ours, because rows
		// stretch to the list box viewport. Position is the list box's business.
		CSWGuiExtent row = GetExtent();
		row.left = 0;
		row.top = 0;
		row.width = rowWidth;
		_SetExtent(&row);

		SetStoredValue(value);
		RefreshLabel();
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

	// SetCurValue, SetMaxValue and Load all tail-call the virtual SetExtent passing
	// &control.extent, so the argument aliases the field the base is about to write.
	// Copy first, or the row loses height on every value change.
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

		// Move, do not re-Load: the art is already baked in.
		if (nameText) {
			nameText->SetExtent(&nameExtent);
		}
	}

	// CSWGuiSlider::Draw handles the border swap and the thumb; this adds the name.
	// Runs every frame: wrappers are cached members, never fetched here.
	void _Draw(float alpha) {
		CSWGuiSlider::Draw(alpha);
		if (nameText) {
			nameText->Draw(alpha);
		}
	}

	// Mirrors CSWGuiSlider::HandleMouseCapturedMovement, which we cannot call: it
	// recomputes the position in the wrong space for a list box row.
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
		const int margin = row->height * BOTTOM_MARGIN_PERCENT / 100;
		const int available = row->height - margin;
		const int trackHeight = (available < TRACK_HEIGHT) ? available : TRACK_HEIGHT;
		const int nameHeight = available - trackHeight;

		*outName  = { row->left, row->top, row->width, nameHeight };
		*outTrack = { row->left, row->top + nameHeight, row->width, trackHeight };
	}
};
