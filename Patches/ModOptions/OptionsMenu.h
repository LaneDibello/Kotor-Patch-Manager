#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"
#include "ModOptionFunctions.h"
#include "ModOptionIni.h"
#include "ModOptionsConfig.h"
#include "OptionsEditBox.h"

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
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"

#include <string>
#include <vector>

class OptionsMenu : public CSWGuiPanel {
public:
	// By value: ModOptions deletes and reloads its configs on refresh.
	ModOptionsConfig config;

	CSWGuiManager* guiManager;

	CSWGuiLabel titleLabel;
	CSWGuiListBox optionsListBox;
	CSWGuiLabel descriptionLabel;
	CSWGuiListBox descriptionListBox;
	CSWGuiButton backButton;
	CSWGuiButton defaultButton;

	// Authoritative option state, indexed like config.options, in INI text form.
	std::vector<std::string> values;

	// Wrappers for the Text options currently in optionsListBox. Each owns a vtable
	// override on its game object, so it has to outlive that object and be torn down
	// before the list box destroys it -- see releaseEditBoxes().
	std::vector<OptionsEditBox*> editBoxes;

	// Control the last suppressed SetActiveControl was for, so the per-frame
	// suppression logs once instead of once per mouse-move.
	void* lastSuppressedFor = nullptr;

	//Callbacks
	void onBack(void* control) {
		debugLog("[ModOptions] Back Button Pressed");
		releaseKeyboardFocus();
		_HandleInputEvent(CSWGuiControl::BButton, 1);
	}

	void setEditFocus(void* control) {
		debugLog("[ModOptions] AButton on %s (active = %s)",
		         describe(control), describe(activeControlPtr()));
		releaseKeyboardFocus(control);

		// SetFocus dispatches HandleFocusChange through the vtable, so this lands in
		// OptionsEditBox::_HandleFocusChange, which hands us the active control.
		CSWGuiEditBox editBox(control);
		editBox.SetFocus();
	}

	// Drops keyboard focus from any edit box other than `keep`, and points the panel
	// back at the list box.
	//
	// The menu has to drive this. CSWGuiEditbox clears GuiManager->focused_edit_box
	// only from its own HandleFocusChange(0), and the only thing that would call that
	// is a parent panel switching active control -- which never happens here, because
	// every option is parented to the list box, whose AsSWGuiPanel is null. Left
	// alone, the first edit box clicked would keep eating keystrokes forever.
	void releaseKeyboardFocus(void* keep = nullptr) {
		bool released = false;
		for (OptionsEditBox* box : editBoxes) {
			if (box->IsFocused() && box->GetPtr() != keep) {
				debugLog("[ModOptions]   releasing focus from %s (keep = %s)",
				         describe(box->GetPtr()), describe(keep));
				box->ReleaseFocus();
				released = true;
			}
		}

		if (released && !keep) {
			SetActiveControl(&optionsListBox, 0);
			debugLog("[ModOptions]   active_control parked back on optionsListBox");
		}
	}

	// CSWGuiManager::HandleKeyPress only routes a keystroke when the top modal's
	// active control IS the focused edit box, so while one of ours has focus the
	// panel has to keep pointing at it.
	//
	// The pressure against that comes from the list box: on every mouse move it takes
	// focus (it is the control the manager's hit check lands on), and
	// CSWGuiControl::HandleFocusChange then walks up to us and claims active_control.
	// The list box is a direct panel child, so unlike its rows that walk succeeds.
	// Refuse it while an edit box inside that same list box is focused.
	void _SetActiveControl(void* control, int playSound) {
		// HandleMouseMove drives this every mouse-move frame while an edit box is
		// focused, so the suppressed path stays allocation- and log-free: report the
		// first one of each run and go quiet until the situation changes.
		if (focusedEditBox() && control == optionsListBox.GetPtr()) {
			if (lastSuppressedFor != control) {
				lastSuppressedFor = control;
				debugLog("[ModOptions] SetActiveControl from %p: -> optionsListBox SUPPRESSED, "
				         "keeping edit box focused (silenced)", LastSetActiveControlCaller());
			}
			return;
		}
		lastSuppressedFor = nullptr;

		void* current = activeControlPtr();
		debugLog("[ModOptions] SetActiveControl from %p: %s -> %s (playSound %i)%s",
		         LastSetActiveControlCaller(), describe(current), describe(control), playSound,
		         current == control ? " [game would early-out]" : "");

		releaseKeyboardFocus(control);

		// The wrapper calls the game's function address directly, so this does not
		// re-enter the override.
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

	void onDefault(void* control) {
		debugLog("[ModOptions] Default Button Pressed");
		releaseKeyboardFocus();

		// Restore all options to their default states
		for (const ModOption& option : config.options) {
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
		debugLog("[ModOptions] AButton on %s (active = %s)",
		         describe(control), describe(activeControlPtr()));
		releaseKeyboardFocus(control);

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
			const bool wasOn = (values[index] == "1");
			value = wasOn ? "0" : "1";
			CSWGuiButtonToggle toggle(control);
			toggle.SetSelected(wasOn ? 0 : 1);
			break;
		}
		case ModOptionType::Slider:
			// TODO
			return;
		case ModOptionType::List:
			// TODO
			return;
		case ModOptionType::Text: {
			CSWGuiEditBox editBox(control);
			CSWGuiEditText* editText = editBox.GetEditText();
			if (!editText) {
				return;
			}
			CExoString* newValue = editText->GetString();
			if (newValue) {
				value = newValue->GetCStr();
				delete newValue;
			}
			delete editText;
			break;
		}
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
		debugLog("[ModOptions] HoverEnter on %s (active = %s)",
		         describe(control), describe(activeControlPtr()));

		CSWGuiControl hovered(control);

		size_t index = (size_t)hovered.GetCustomValue();
		const ModOption* opt = config.GetOption(index);
		if (!opt) {
			debugLog("[ModOptions] hovered option has out-of-range custom value %u", (unsigned)index);
			return;
		}

		SetControlText(&descriptionLabel, opt->GetDescription());

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
		releaseEditBoxes();
		ThunkRegistry::Unregister(this);
	}

private:
	// DEBUG: turns a raw game control pointer into something readable in the log.
	// Rotates through a few buffers so several calls can share one debugLog.
	const char* describe(void* control) {
		static char buffers[4][64];
		static int next = 0;
		char* buffer = buffers[next];
		next = (next + 1) % 4;

		if (!control) {
			return "(null)";
		}
		if (control == optionsListBox.GetPtr())     return "optionsListBox";
		if (control == descriptionListBox.GetPtr()) return "descriptionListBox";
		if (control == backButton.GetPtr())         return "backButton";
		if (control == defaultButton.GetPtr())      return "defaultButton";
		if (control == titleLabel.GetPtr())         return "titleLabel";
		if (control == descriptionLabel.GetPtr())   return "descriptionLabel";

		for (size_t i = 0; i < editBoxes.size(); ++i) {
			if (editBoxes[i]->GetPtr() == control) {
				snprintf(buffer, 64, "editBox[%u]%s", (unsigned)i,
				         editBoxes[i]->IsFocused() ? " FOCUSED" : "");
				return buffer;
			}
		}

		// A list row we did not build a wrapper for: a toggle.
		CSWGuiControl row(control);
		snprintf(buffer, 64, "row?custom=%u", (unsigned)row.GetCustomValue());
		return buffer;
	}

	void* activeControlPtr() {
		CSWGuiControl* active = GetActiveControl();
		void* ptr = active ? active->GetPtr() : nullptr;
		delete active;
		return ptr;
	}

	// Tears down the Text-option wrappers. Each destructor restores the game vtable
	// it overrode, so this must run while those game objects are still alive -- i.e.
	// before CSWGuiListBox::ClearItems.
	//
	// ReleaseOwnership first: the game objects went to AddControls and whoever frees
	// them, it is not us. That leaves the wrapper doing exactly what it did before it
	// grew a vtable override -- restore, and hands off the game memory untouched.
	void releaseEditBoxes() {
		// Keyboard mode is global; drop it before the boxes holding it go away.
		releaseKeyboardFocus();

		for (OptionsEditBox* box : editBoxes) {
			box->ReleaseOwnership();
			delete box;
		}
		editBoxes.clear();
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
		releaseEditBoxes();

		optionsListBox.ClearItems();
		values.assign(config.OptionCount(), std::string());

		CSWGuiControl* protoItem = optionsListBox.GetProtoItem();
		if (!protoItem) {
			debugLog("[ModOptions] LB_OPTIONS has no proto item");
			return;
		}

		// Will likely make this more generic in the future...
		CSWGuiButtonToggle proto(protoItem->GetPtr());
		delete protoItem;

		// Caller-owned wrappers, and the same for every option, so hoisted out.
		CSWGuiText* protoText = proto.GetText();
		CSWGuiBorder* protoBorder = proto.GetBorder();
		CSWGuiBorder* protoHilight = proto.GetHilight();
		CSWGuiBorder* protoSelectedBorder = proto.GetSelectedBorder();
		CSWGuiBorder* protoHilightSelected = proto.GetHilightSelectedBorder();
		CSWGuiTextParams* textParams = protoText ? protoText->GetTextParams() : nullptr;
		CSWGuiBorderParams* borderParams = protoBorder ? protoBorder->GetBorderParams() : nullptr;
		CSWGuiBorderParams* hilightParams = protoHilight ? protoHilight->GetBorderParams() : nullptr;
		CSWGuiBorderParams* selectedParams = protoSelectedBorder ? protoSelectedBorder->GetBorderParams() : nullptr;
		CSWGuiBorderParams* hilightSelectedParams = protoHilightSelected ? protoHilightSelected->GetBorderParams() : nullptr;

		CSWGuiExtent optionExtent;
		optionExtent.top = 0;
		optionExtent.left = 0;
		optionExtent.width = optionsListBox.GetViewportWidth() - 2 * optionsListBox.GetPadding();
		optionExtent.height = proto.GetExtent().height;

		CExoArrayList<CSWGuiControl*> listOptions;
		const std::vector<ModOption>& options = config.GetOptions();
		for (size_t i = 0; i < options.size(); ++i) {
			const std::string value = ResolveOptionValue(options[i]);
			values[i] = value;

			switch (options[i].type) {
			case ModOptionType::Toggle: {
				CSWGuiButtonToggle* toggle = new CSWGuiButtonToggle();
				toggle->SetOptionsCheckbox();

				toggle->Initialize(&optionExtent, textParams,
					borderParams, hilightParams,
					selectedParams, hilightSelectedParams);
				toggle->SetToggleEvent((CSWGuiControl::GuiEvent)-1);
				toggle->SetSelected((value == "1") ? 1 : 0);
				SetControlText(toggle, options[i].name);

				toggle->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::onOption>());
				toggle->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				toggle->SetCustomValue((DWORD)i);

				listOptions.Add(toggle);
				break;
			}
			case ModOptionType::Slider:
				// TODO
				break;
			case ModOptionType::List:
				// TODO
				break;
			case ModOptionType::Text:
				OptionsEditBox* editBox = new OptionsEditBox(this);

				CResRef corner("border2");
				CResRef edge("border1");
				CResRef fill("dialog3");
				borderParams->SetCornerImage(&corner, 1);
				borderParams->SetEdgeImage(&edge, 1);
				borderParams->SetFillImage(&fill, 1);
				editBox->Initialize(&optionExtent, textParams, borderParams);
				CExoString textValue(const_cast<char*>(value.c_str()));
				editBox->GetEditText()->SetText(&textValue);

				editBox->AddEvent(CSWGuiControl::AButton, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::setEditFocus>());
				editBox->AddEvent(CSWGuiControl::HoverEnter, this,
					memberThunkAddr<OptionsMenu, &OptionsMenu::SetDescription>());

				editBox->SetCustomValue(DWORD(i));

				editBoxes.push_back(editBox);
				listOptions.Add(editBox);
				break;
			}
		}

		delete hilightParams;
		delete borderParams;
		delete textParams;
		delete protoHilight;
		delete protoBorder;
		delete protoText;

		if (listOptions.GetSize() == 0) {
			debugLog("[ModOptions] `%s` produced no usable controls", config.GetName().c_str());
			return;
		}

		debugLog("[ModOptions] `%s` produced %i options", config.GetName().c_str(), listOptions.GetSize());

		optionsListBox.AddControls(&listOptions, 1, 0, 0);
	}

	void _HandleInputEvent(int event, int doPanelEvents) {
		debugLog("[ModOptions] OptionsMenu _HandleInputEvent (%i, %i)", event, doPanelEvents);
		if (doPanelEvents && guiManager) {
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

		HandleInputEvent(event, doPanelEvents);
	}
};
