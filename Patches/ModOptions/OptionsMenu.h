#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"
#include "ModOptionFunctions.h"
#include "ModOptionIni.h"
#include "ModOptionsConfig.h"

#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiButtonToggle.h"
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

	//Callbacks
	void onBack(void* control) {
		debugLog("[ModOptions] Back Button Pressed");
		_HandleInputEvent(CSWGuiControl::BButton, 1);
	}

	void onDefault(void* control) {
		debugLog("[ModOptions] Default Button Pressed");

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
		case ModOptionType::Text:
			// TODO
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
		descriptionListBox.AddControls(&descriptionItems, 1, 0, 0);
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
		backButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<OptionsMenu, &OptionsMenu::onBack>());

		this->OverrideHandleInputEvent(memberFuncAddr(&OptionsMenu::_HandleInputEvent));
	}

	~OptionsMenu() {
		ThunkRegistry::Unregister(this);
	}

private:
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

		optionsListBox.ClearItems();
		values.assign(config.OptionCount(), std::string());

		CSWGuiControl* protoItem = optionsListBox.GetProtoItem();
		if (!protoItem) {
			debugLog("[ModOptions] LB_OPTIONS has no proto item");
			return;
		}

		CSWGuiButton proto(protoItem->GetPtr());
		delete protoItem;

		// Caller-owned wrappers, and the same for every option, so hoisted out.
		CSWGuiText* protoText = proto.GetText();
		CSWGuiBorder* protoBorder1 = proto.GetBorder1();
		CSWGuiBorder* protoBorder2 = proto.GetBorder2();
		CSWGuiTextParams* textParams = protoText ? protoText->GetTextParams() : nullptr;
		CSWGuiBorderParams* border1Params = protoBorder1 ? protoBorder1->GetBorderParams() : nullptr;
		CSWGuiBorderParams* border2Params = protoBorder2 ? protoBorder2->GetBorderParams() : nullptr;

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

				toggle->Initialize(&optionExtent, textParams,
					border1Params, border2Params,
					border1Params, border2Params);
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
				// TODO
				break;
			}
		}

		delete border2Params;
		delete border1Params;
		delete textParams;
		delete protoBorder2;
		delete protoBorder1;
		delete protoText;

		if (listOptions.GetSize() == 0) {
			debugLog("[ModOptions] `%s` produced no usable controls", config.GetName().c_str());
			return;
		}

		optionsListBox.AddControls(&listOptions, 1, 0, 0);
		optionsListBox.SetSelectedControl(0, 0);
	}

	void _HandleInputEvent(int event, int doPanelEvents) {
		if (doPanelEvents && guiManager) {
			switch (event) {
			case CSWGuiControl::BButton:
				guiManager->PlayGuiSound(0);
				guiManager->PopModalPanel();
				break;
			default:
				break;
			}
		}

		HandleInputEvent(event, doPanelEvents);
	}
};
