#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"
#include "ModOptionFunctions.h"
#include "ModOptionIni.h"
#include "ModOptionsConfig.h"
#include "OptionsEditBox.h"
#include "OptionsLayout.h"
#include "OptionsListBox.h"
#include "OptionsListSelect.h"
#include "OptionsSlider.h"

#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiButtonToggle.h"
#include "GameAPI/CSWGuiEditBox.h"
#include "GameAPI/CSWGuiEditText.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiListBox.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiSlider.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"

#include <cstdlib>
#include <string>
#include <vector>

class OptionsMenu : public CSWGuiPanel {
public:
	ModOptionsConfig config;

	CSWGuiManager* guiManager;

	CSWGuiLabel titleLabel;
	OptionsListBox optionsListBox;
	CSWGuiLabel descriptionLabel;
	CSWGuiListBox descriptionListBox;
	CSWGuiButton backButton;
	CSWGuiButton defaultButton;

	// Authoritative option state, indexed like config.options, in INI text form.
	std::vector<std::string> values;


	// Wrappers for the Text options currently in optionsListBox
	std::vector<OptionsEditBox*> editBoxes;

	// Wrappers for the Slider options currently in optionsListBox
	std::vector<OptionsSlider*> sliders;

	// Wrappers for the List options currently in optionsListBox
	std::vector<OptionsListSelect*> listSelects;

	// Control the last suppressed SetActiveControl was for

	//Callbacks
	void onBack(void* control) {
		releaseKeyboardFocus();
		_HandleInputEvent(CSWGuiControl::BButton, 1);
	}

	void setEditFocus(void* control) {
		releaseKeyboardFocus(control);

		CSWGuiEditBox editBox(control);
		editBox.SetFocus();
	}

	// Drops keyboard focus from any edit box other than `keep`
	void releaseKeyboardFocus(void* keep = nullptr) {
		bool released = false;
		for (OptionsEditBox* box : editBoxes) {
			if (box->IsFocused() && box->GetPtr() != keep) {
				box->ReleaseFocus();
				released = true;
			}
		}

		if (released && !keep) {
			SetActiveControl(&optionsListBox, 0);
		}
	}

	void _SetActiveControl(void* control, int playSound) {
		// HandleMouseMove drives this every mouse-move frame while an edit box is
		// focused. Letting it through would hand active_control back to the list box,
		// and HandleKeyPress only routes keys while active_control IS the focused box.
		if (focusedEditBox() && control == optionsListBox.GetPtr()) {
			return;
		}

		releaseKeyboardFocus(control);

		CSWGuiControl incoming(control);
		SetActiveControl(&incoming, playSound);
	}

	OptionsEditBox* focusedEditBox() {
		for (OptionsEditBox* box : editBoxes) {
			if (box->IsFocused()) {
				return box;
			}
		}
		return nullptr;
	}

	// The retained wrapper for a slider's game pointer. Re-wrapping the raw pointer
	// would read cur_value fine but lose the option's `min` and its label, both of
	// which live on the subclass.
	OptionsSlider* findSlider(void* control) {
		for (OptionsSlider* slider : sliders) {
			if (slider->GetPtr() == control) {
				return slider;
			}
		}
		return nullptr;
	}

	// Same reason as findSlider: a re-wrapped raw pointer would lose the option's
	// `choices` and the index it is currently showing.
	OptionsListSelect* findListSelect(void* control) {
		for (OptionsListSelect* listSelect : listSelects) {
			if (listSelect->GetPtr() == control) {
				return listSelect;
			}
		}
		return nullptr;
	}

	// The arrow keys, for the row that is the list box's active control. A keypress
	// fires these twice, and a step is not idempotent.
	void onListLeft(void* control) {
		if (currentInputPhase == 0) {
			return;
		}
		stepListSelect(control, -1);
	}
	void onListRight(void* control) {
		if (currentInputPhase == 0) {
			return;
		}
		stepListSelect(control, 1);
	}

	// Step commits through commitOption itself, so there is nothing to do after.
	void stepListSelect(void* control, int delta) {
		releaseKeyboardFocus(control);

		OptionsListSelect* listSelect = findListSelect(control);
		if (!listSelect) {
			debugLog("[ModOptions] no list select wrapper registered for %X", control);
			return;
		}
		listSelect->Step(delta);
	}

	// A button stores nothing, so it never goes through commitOption -- a press is
	// the whole option. Fires twice per keypress, and a mod's handler need not be
	// idempotent, so the tail half is dropped.
	void onButton(void* control) {
		if (currentInputPhase == 0) {
			return;
		}
		releaseKeyboardFocus(control);

		CSWGuiControl button(control);
		size_t index = (size_t)button.GetCustomValue();
		const ModOption* opt = config.GetOption(index);
		if (!opt) {
			debugLog("[ModOptions] mod option button has out-of-range custom value %u", (unsigned)index);
			return;
		}

		// A button carries no value; the handler gets its key and an empty string.
		if (!InvokeModOptionHandler(*opt, std::string())) {
			debugLog("[ModOptions] `%s` could not run `%s`",
				opt->GetName().c_str(), opt->GetFunction().c_str());
		}
	}

	void onDefault(void* control) {
		releaseKeyboardFocus();

		// Restore all options to their default states
		for (const ModOption& option : config.options) {
			// A button has no default, and firing its function is not a restore.
			if (option.type == ModOptionType::Button) {
				continue;
			}
			if (option.HasIni()) {
				WriteOptionValue(option, option.defaultString);
			}
			if (option.HasFunction()) {
				InvokeModOptionHandler(option, option.defaultString);
			}
		}

		populateOptionsListBox();
	}

	void onOption(void* control) {
		// A keypress fires this twice. Toggle derives its new value from the stored
		// one, so acting on both halves flips it straight back.
		if (currentInputPhase == 0) {
			return;
		}
		releaseKeyboardFocus(control);
		commitOption(control);
	}

	// Reads a control's current state and writes it through to values/INI/handler.
	void commitOption(void* control) {
		CSWGuiControl option(control);
		size_t index = (size_t)option.GetCustomValue();
		if (index >= config.OptionCount() || index >= values.size()) {
			debugLog("[ModOptions] mod option button has out-of-range custom value %u", (unsigned)index);
			return;
		}

		const ModOption* opt = config.GetOption(index);
		if (!opt) {
			return;
		}

		// Get the state of the control
		std::string value = "";
		switch (opt->type) {
		case ModOptionType::Toggle: {
			const bool wasOn = ModOptionsConfigDetail::IsOn(values[index]);
			value = wasOn ? "0" : "1";
			CSWGuiButtonToggle toggle(control);
			toggle.SetSelected(wasOn ? 0 : 1);
			break;
		}
		case ModOptionType::Slider: {
			OptionsSlider* slider = findSlider(control);
			if (!slider) {
				debugLog("[ModOptions] no slider wrapper registered for %X", control);
				return;
			}
			// The game has already moved the thumb by the time an event reaches us.
			slider->RefreshLabel();
			value = std::to_string(slider->StoredValue());
			break;
		}
		case ModOptionType::List: {
			OptionsListSelect* listSelect = findListSelect(control);
			if (!listSelect) {
				debugLog("[ModOptions] no list select wrapper registered for %X", control);
				return;
			}
			// Step has already moved the control by the time this runs.
			value = listSelect->Value();
			break;
		}
		case ModOptionType::Button:
			// Never reached: a button's only event goes straight to onButton.
			return;
		case ModOptionType::Text: {
			CSWGuiEditBox editBox(control);
			CSWGuiEditText* editText = editBox.GetEditText();
			if (!editText) {
				return;
			}

			CSWGuiTextParams* params = editText->GetTextParams();
			CExoString* displayed = params ? params->GetText() : nullptr;
			value = displayed ? displayed->ToStdString() : std::string();

			delete displayed;
			delete params;
			delete editText;
			break;
		}
		}

		// Every input path arrives twice, and a mod's handler need not be idempotent.
		// A toggle always changes value, so it is unaffected.
		if (values[index] == value) {
			return;
		}
		values[index] = value;

		// INI first, so a handler that re-reads its settings sees the new value.
		if (opt->HasIni()) {
			WriteOptionValue(*opt, value);
		}
		if (opt->HasFunction()) {
			InvokeModOptionHandler(*opt, value);
		}
	}

	void SetDescription(void* control) {
		CSWGuiControl hovered(control);

		size_t index = (size_t)hovered.GetCustomValue();
		const ModOption* opt = config.GetOption(index);
		if (!opt) {
			debugLog("[ModOptions] hovered option has out-of-range custom value %u", (unsigned)index);
			return;
		}

		// A list select has no room for the option's name -- its own text is the
		// current choice -- so the name leads the description instead.
		std::string description = opt->GetDescription();
		if (opt->type == ModOptionType::List) {
			description = description.empty()
				? opt->GetName()
				: opt->GetName() + "\n" + description;
		}
		SetControlText(&descriptionLabel, description);

		descriptionListBox.ClearItems();

		CSWGuiText* labelText = descriptionLabel.GetText();
		if (labelText) {
			CSWGuiExtent labelExtent = descriptionListBox.GetExtent();
			labelExtent.height = labelText->GetIdealHeight();
			descriptionLabel.SetExtent(labelExtent);
			delete labelText;
		}

		// Only the CExoArrayList overload marshals elements to game pointers.
		CExoArrayList<CSWGuiControl*> descriptionItems;
		descriptionItems.Add(&descriptionLabel);
		descriptionListBox.AddControls(&descriptionItems, 0, 0, 0);
		descriptionListBox.SetActiveControl(&descriptionLabel, 0);
	}

	OptionsMenu(CSWGuiManager* manager, const ModOptionsConfig& menuConfig) :
		CSWGuiPanel(manager),
		config(menuConfig),
		guiManager(manager),
		titleLabel(),
		optionsListBox(),
		descriptionLabel(),
		descriptionListBox(),
		backButton(),
		defaultButton()
	{
		if (!config.loaded) {
			debugLog("[ModOptions] OptionsMenu built without a loaded config");
			return;
		}

		// Before any AddEvent: the thunks look us up by game pointer.
		ThunkRegistry::Register(this);

		CResRef guiResref("modoptionmenu");
		this->StartLoadFromLayout(&guiResref);
		CExoString titleTag("LBL_TITLE");
		this->InitControl(&titleLabel, &titleTag, 1);
		CExoString optionsTag("LB_OPTIONS");
		this->InitControl(&optionsListBox, &optionsTag, 1);
		CExoString descriptionTag("LB_DESC");
		this->InitControl(&descriptionListBox, &descriptionTag, 1);
		CExoString backTag("BTN_BACK");
		this->InitControl(&backButton, &backTag, 1);
		CExoString defaultTag("BTN_DEFAULT");
		this->InitControl(&defaultButton, &defaultTag, 1);
		this->StopLoadFromLayout();

		// After the layout: the override goes on the control the layout populated.
		optionsListBox.HookMouse(this);

		SetControlText(&titleLabel, config.GetName());

		//Description Logic
		CSWGuiControl* descProtoItem = descriptionListBox.GetProtoItem();
		if (descProtoItem) {
			CSWGuiLabel descProto(descProtoItem->GetPtr());
			CSWGuiExtent descExtent = descProto.GetExtent();
			descriptionLabel.Initialize(&descExtent, &descProto, 1.0f);
			delete descProtoItem;
		}

		populateOptionsListBox();

		defaultButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<OptionsMenu, &OptionsMenu::onDefault>());
		defaultButton.SetControlBitFlag(2, false);
		backButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<OptionsMenu, &OptionsMenu::onBack>());
		backButton.SetControlBitFlag(2, false);

		this->OverrideHandleInputEvent(memberFuncAddr(&OptionsMenu::_HandleInputEvent));
		this->OverrideSetActiveControl(memberFuncAddr(&OptionsMenu::_SetActiveControl));

		SetActiveControl(&optionsListBox, 0);
		CSWGuiControl* first = optionsListBox.GetControl(0);
		optionsListBox.SetActiveControl(first, 0);
	}

	~OptionsMenu() {
		releaseControls();
		ThunkRegistry::Unregister(this);
	}

private:
	// Tears down the wrappers we hold on to across a repopulate.
	void releaseControls() {
		// Keyboard mode is global; drop it before the boxes holding it go away.
		releaseKeyboardFocus();

		for (OptionsEditBox* box : editBoxes) {
			box->ReleaseOwnership();
			delete box;
		}
		editBoxes.clear();

		for (OptionsSlider* slider : sliders) {
			slider->ReleaseOwnership();
			delete slider;
		}
		sliders.clear();

		// ReleaseOwnership covers the row itself, which the list box frees; the two
		// arrows it owns were never handed over, and its destructor frees those.
		for (OptionsListSelect* listSelect : listSelects) {
			listSelect->ReleaseOwnership();
			delete listSelect;
		}
		listSelects.clear();
	}

	// Height comes from the layout, width from the list box, position from AddControls.
	// LayoutExtent, not CSWGuiObject::SetExtent -- the latter only writes the field
	// and would leave the border and text at the template's width.
	void sizeRow(CSWGuiControl* control, int rowWidth) {
		if (!control) {
			return;
		}
		CSWGuiExtent row = control->GetExtent();
		row.left = 0;
		row.top = 0;
		row.width = rowWidth;
		control->LayoutExtent(&row);
	}

	// GetText/GetTextParams hand back caller-owned wrappers.
	template <typename ControlT>
	void SetControlText(ControlT* control, const std::string& text) {
		if (!control) {
			return;
		}

		CSWGuiText* textObject = control->GetText();
		if (!textObject) {
			return;
		}

		CSWGuiTextParams* textParams = textObject->GetTextParams();
		if (textParams) {
			CExoString value(const_cast<char*>(text.c_str()));
			textParams->SetText(&value);
			delete textParams;
		}
		delete textObject;
	}

	void populateOptionsListBox() {
		if (!config.loaded) {
			return;
		}

		// Before ClearItems: each wrapper's destructor writes the game's vtable back
		// into its game object, which ClearItems is about to destroy.
		releaseControls();

		optionsListBox.ClearItems();
		values.assign(config.OptionCount(), std::string());

		// Scoped to the build. AddPanel takes ownership of the panel and our wrapper
		// is never destroyed, so a longer-lived layout would hold its Demand forever
		// and every later open of this menu would read an empty GFF.
		OptionsLayout layout;
		if (!layout.Open("modoptionmenu")) {
			debugLog("[ModOptions] no layout to build option rows from");
			return;
		}

		// Every row type styles and sizes itself from its own template control in
		// the layout, so the only thing left to decide here is the width.
		const int rowWidth = optionsListBox.GetViewportWidth() - 2 * optionsListBox.GetPadding();

		CExoArrayList<CSWGuiControl*> listOptions;
		const std::vector<ModOption>& options = config.GetOptions();
		for (size_t i = 0; i < options.size(); ++i) {
			const std::string value = ResolveOptionValue(options[i]);
			values[i] = value;

			switch (options[i].type) {
			case ModOptionType::Toggle: {
				CSWGuiButtonToggle* toggle = new CSWGuiButtonToggle();
				if (!layout.Load(toggle, this, "OPT_TOGGLE")) {
					delete toggle;
					break;
				}

				toggle->SetOptionsCheckbox();
				sizeRow(toggle, rowWidth);

				toggle->SetToggleEvent((CSWGuiControl::GuiEvent)-1);

				toggle->SetSelected(ModOptionsConfigDetail::IsOn(value) ? 1 : 0);
				SetControlText(toggle, options[i].name);

				toggle->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				toggle->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				toggle->SetCustomValue((DWORD)i);

				listOptions.Add(toggle);
				break;
			}
			case ModOptionType::Slider: {
				OptionsSlider* slider = new OptionsSlider(this);

				if (!layout.Load(slider, this, "OPT_SLIDER")) {
					delete slider;
					break;
				}
				slider->Configure(rowWidth, options[i], atoi(value.c_str()));

				// The gamma slider's wiring. HandleInputEvent sets cur_value before
				// dispatching these, so it is already current by the time we commit.
				slider->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				slider->AddEvent(CSWGuiControl::LeftArrow, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				slider->AddEvent(CSWGuiControl::RightArrow, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				slider->AddEvent(CSWGuiSlider::EVENT_TRACK_UP, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				slider->AddEvent(CSWGuiSlider::EVENT_TRACK_DOWN, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				slider->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				slider->SetCustomValue((DWORD)i);

				sliders.push_back(slider);
				listOptions.Add(slider);
				break;
			}
			case ModOptionType::List: {
				OptionsListSelect* listSelect = new OptionsListSelect(this);

				if (!layout.Load(listSelect, this, "OPT_LIST")
					|| !listSelect->LoadChildren(layout, this)) {
					delete listSelect;
					break;
				}
				listSelect->Configure(rowWidth, options[i], value);

				// No AButton: a click on the label itself must not change a setting.
				// The arrows are driven from OptionsListBox::_HandleLMouseDown.
				listSelect->AddEvent(CSWGuiControl::LeftArrow, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onListLeft>());
				listSelect->AddEvent(CSWGuiControl::RightArrow, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onListRight>());
				listSelect->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				listSelect->SetCustomValue((DWORD)i);

				listSelects.push_back(listSelect);
				listOptions.Add(listSelect);
				break;
			}
			case ModOptionType::Text: {
				OptionsEditBox* editBox = new OptionsEditBox(this, this);

				if (!layout.Load(editBox, this, "OPT_TEXT")) {
					delete editBox;
					break;
				}
				editBox->Configure(rowWidth, options[i].name, value);

				editBox->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::setEditFocus>());
				editBox->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				editBox->SetCustomValue(DWORD(i));

				editBoxes.push_back(editBox);
				listOptions.Add(editBox);
				break;
			}
			case ModOptionType::Button: {
				CSWGuiButton* button = new CSWGuiButton();

				if (!layout.Load(button, this, "OPT_BUTTON")) {
					delete button;
					break;
				}
				sizeRow(button, rowWidth);
				SetControlText(button, options[i].name);

				button->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onButton>());
				button->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				button->SetCustomValue((DWORD)i);

				listOptions.Add(button);
				break;
			}
			}
		}

		if (listOptions.GetSize() == 0) {
			debugLog("[ModOptions] `%s` produced no usable controls", config.GetName().c_str());
			return;
		}

		debugLog("[ModOptions] `%s` produced %i options", config.GetName().c_str(), listOptions.GetSize());

		// Each row type takes its height from its own template, so heights vary.
		optionsListBox.AddControls(&listOptions, 1, 0, 1);

	}

	// The manager delivers every key twice -- phase 1 then phase 0. A panel opened
	// on phase 1 becomes modal-stack top in time to receive the phase 0 tail of the
	// very keypress that opened it. Drop input until a phase 1 arrives.
	bool sawInputStart = false;

	// Phase of the key event being dispatched, for the AddEvent callbacks -- they
	// are handed only a control, and the game fires them on both phases. Mouse
	// paths never come through the panel, hence the reset to 1.
	int currentInputPhase = 1;

	bool ownsInput(int phase) {
		if (sawInputStart) {
			return true;
		}
		if (phase == 0) {
			return false;
		}
		sawInputStart = true;
		return true;
	}

	void _HandleInputEvent(int event, int inputPhase) {
		if (!ownsInput(inputPhase)) {
			return;
		}
		currentInputPhase = inputPhase;
		if (inputPhase && guiManager) {
			switch (event) {
			case CSWGuiControl::BButton:
			{
				releaseKeyboardFocus();
				guiManager->PlayGuiSound(0);
				guiManager->PopModalPanel();
				// TODO: properly label these bit flags
				SetBitFlags((GetBitFlags() & ~0x300) | 0x400);
				break;
			}
			default:
				break;
			}
		}

		HandleInputEvent(event, inputPhase);
		currentInputPhase = 1;
	}
};

// Defined here rather than in OptionsEditBox.h: the edit box only forward-declares
// OptionsMenu, and both headers land in the same translation unit.
inline void OptionsEditBox::CommitToMenu() {
	if (menu) {
		menu->commitOption(GetPtr());
	}
}

inline void OptionsEditBox::ReleaseToMenu() {
	if (menu) {
		menu->releaseKeyboardFocus();
	}
}

inline void OptionsSlider::CommitToMenu() {
	if (menu) {
		menu->commitOption(GetPtr());
	}
}

inline void OptionsListSelect::CommitToMenu() {
	if (menu) {
		menu->commitOption(GetPtr());
	}
}

inline bool OptionsListSelect::RowMouseCoords(int* outX, int* outY) {
	return menu ? menu->optionsListBox.LocalContentCoords(outX, outY) : false;
}

inline void OptionsListSelect::PlayStepSound() {
	// Sound 1, as vanilla's OnAnisotropyLeft/Right play.
	if (menu && menu->guiManager) {
		menu->guiManager->PlayGuiSound(1);
	}
}

inline bool OptionsListBox::LocalContentCoords(int* outX, int* outY) {
	if (!menu) {
		return false;
	}
	menu->GetLocalMouseCoords(outX, outY);
	*outX -= GetViewportX();
	*outY -= GetViewportY();
	return true;
}

inline void OptionsListBox::_HandleLMouseDown() {
	// Base first, for the selection bookkeeping and its own mouse capture -- which
	// it keeps, so the drag below is driven from here rather than from the slider.
	CSWGuiListBox::HandleLMouseDown();
	dragging = nullptr;

	int index = 0;
	CSWGuiControl* hit = HitCheckMouseLocal(&index);
	if (!hit) {
		return;
	}
	OptionsSlider* slider = menu ? menu->findSlider(hit->GetPtr()) : nullptr;
	OptionsListSelect* listSelect = menu ? menu->findListSelect(hit->GetPtr()) : nullptr;
	delete hit;

	int x = 0, y = 0;
	if ((!slider && !listSelect) || !LocalContentCoords(&x, &y)) {
		return;
	}

	if (slider) {
		// The two track events are the ones CSWGuiSlider::HandleLMouseDown would have
		// raised; they step the value and reach commitOption through the events the row
		// registered. Only the thumb starts a drag.
		switch (slider->HitTest(x, y)) {
		case 1:
			dragging = slider;
			break;
		case 2:
			slider->HandleInputEvent(CSWGuiSlider::EVENT_TRACK_DOWN, 1);
			break;
		case 3:
			slider->HandleInputEvent(CSWGuiSlider::EVENT_TRACK_UP, 1);
			break;
		default:
			break;
		}
		return;
	}

	// A list select has nothing to drag, so the capture the base took is left alone
	// and the step happens here and now.
	switch (listSelect->HitTest(x, y)) {
	case OptionsListSelect::HIT_LEFT:
		listSelect->Step(-1);
		break;
	case OptionsListSelect::HIT_RIGHT:
		listSelect->Step(1);
		break;
	default:
		break;
	}
}

inline int OptionsListBox::_HandleMouseCapturedMovement(int x, int y) {
	if (!dragging) {
		return CSWGuiListBox::HandleMouseCapturedMovement(x, y);
	}

	// Label only while the button is down; a commit writes an ini file to disk and
	// calls into the owning patch, and this runs every mouse-move frame.
	int localX = 0, localY = 0;
	if (LocalContentCoords(&localX, &localY)) {
		dragging->SetValueFromTrackX(localX);
	}
	return 1;
}

inline void OptionsListBox::_HandleLMouseUp() {
	CSWGuiListBox::HandleLMouseUp();

	if (dragging) {
		dragging->CommitToMenu();
		dragging = nullptr;
	}
}
