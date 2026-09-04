#pragma once
#include "Common.h"
#include "MemberFunctionThunk.h"
#include "ModOptionsConfig.h"
#include "OptionsMenu.h"

#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiExtent.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiListBox.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"

#include <filesystem>
#include <string>
#include <vector>

class ModOptions : public CSWGuiPanel {
public:
	CSWGuiManager* guiManager;

	//Controls
	CSWGuiLabel titleLabel;
	CSWGuiListBox optionsListBox;
	CSWGuiLabel descriptionLabel;
	CSWGuiListBox descriptionListBox;
	CSWGuiButton backButton;
	CSWGuiButton refreshButton;

	std::vector<ModOptionsConfig> modOptionConfigs;

	// Callbacks
	void onModOption(void* control) {
		CSWGuiControl button(control);
		size_t index = (size_t)button.GetCustomValue();
		if (index >= modOptionConfigs.size()) {
			debugLog("[ModOptions] mod option button has out-of-range custom value %u", (unsigned)index);
			return;
		}

		const ModOptionsConfig& config = modOptionConfigs[index];
		debugLog("[ModOptions] selected `%s` (%s)", config.GetName().c_str(), config.GetSourcePath().c_str());

		// AddPanel hands the panel to the game, which frees it.
		guiManager->AddPanel(new OptionsMenu(guiManager, config), 3, 1);
	}
	void onRefresh(void* control) {
		debugLog("[ModOptions] Refresh Button Pressed");
		populateOptionsListBox();
	}
	void onBack(void* control) {
		_HandleInputEvent(CSWGuiControl::BButton, 1);
	}

	ModOptions(CSWGuiManager* manager) :
		CSWGuiPanel(manager),
		guiManager(manager),
		titleLabel(),
		optionsListBox(),
		descriptionLabel(),
		descriptionListBox(),
		backButton(),
		refreshButton()
	{
		ThunkRegistry::Register(this);

		CResRef guiResref("modoptions");
		this->StartLoadFromLayout(&guiResref);
		CExoString titleTag("LBL_TITLE");
		this->InitControl(&titleLabel, &titleTag, 1);
		CExoString optionsTag("LB_OPTIONS");
		this->InitControl(&optionsListBox, &optionsTag, 1);
		CExoString descriptionTag("LB_DESC");
		this->InitControl(&descriptionListBox, &descriptionTag, 1);
		CExoString backTag("BTN_BACK");
		this->InitControl(&backButton, &backTag, 1);
		CExoString refreshTag("BTN_REFRESH");
		this->InitControl(&refreshButton, &refreshTag, 1);
		this->StopLoadFromLayout();

		debugLog("Loaded Mod Options from layout");

		// Set up description ListBox
		CSWGuiControl* descProtoItem = descriptionListBox.GetProtoItem();
		if (!descProtoItem) {
			debugLog("[ModOptions] LB_DESC has no proto item");
		}
		else {
			CSWGuiLabel descProto(descProtoItem->GetPtr());
			CSWGuiExtent descExtent = descProto.GetExtent();
			descriptionLabel.Initialize(&descExtent, &descProto, 1.0f);
			delete descProtoItem;
		}

		populateOptionsListBox();

		refreshButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<ModOptions, &ModOptions::onRefresh>());
		backButton.AddEvent(CSWGuiControl::AButton, this,
			memberThunkAddr<ModOptions, &ModOptions::onBack>());

		this->OverrideHandleInputEvent(memberFuncAddr(&ModOptions::_HandleInputEvent));
	}

	~ModOptions() {
		ThunkRegistry::Unregister(this);
	}

private:
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

	void loadModOptionConfigs() {
		modOptionConfigs.clear();

		std::filesystem::path directory("Mod Options");
		std::error_code ec;
		std::filesystem::directory_iterator entries(directory, ec);
		if (ec) {
			debugLog("[ModOptions] cannot read `%s`: %s", directory.string().c_str(), ec.message().c_str());
			return;
		}

		for (const auto& entry : entries) {
			if (!entry.is_regular_file() || entry.path().extension() != ".toml") {
				continue;
			}

			ModOptionsConfig config = ModOptionsConfig::LoadFromFile(entry.path().string());
			if (!config.loaded) {
				continue;
			}

			modOptionConfigs.push_back(std::move(config));
		}

		debugLog("[ModOptions] loaded %u mod option config(s)", (unsigned)modOptionConfigs.size());
	}
	void populateOptionsListBox() {
		loadModOptionConfigs();

		optionsListBox.ClearItems();

		CSWGuiControl* protoItem = optionsListBox.GetProtoItem();
		if (!protoItem) {
			debugLog("[ModOptions] LB_OPTIONS has no proto item");
			return;
		}

		CSWGuiButton proto(protoItem->GetPtr());
		delete protoItem;

		CSWGuiText* protoText = proto.GetText();
		CSWGuiBorder* protoBorder1 = proto.GetBorder1();
		CSWGuiBorder* protoBorder2 = proto.GetBorder2();
		CSWGuiTextParams* textParams = protoText ? protoText->GetTextParams() : nullptr;
		CSWGuiBorderParams* border1Params = protoBorder1 ? protoBorder1->GetBorderParams() : nullptr;
		CSWGuiBorderParams* border2Params = protoBorder2 ? protoBorder2->GetBorderParams() : nullptr;

		CSWGuiExtent buttonExtent;
		buttonExtent.top = 0;
		buttonExtent.left = 0;
		buttonExtent.width = optionsListBox.GetViewportWidth() - 2 * optionsListBox.GetPadding();
		buttonExtent.height = proto.GetExtent().height;

		CExoArrayList<CSWGuiControl*> listButtons;
		for (size_t i = 0; i < modOptionConfigs.size(); ++i) {
			CSWGuiButton* button = new CSWGuiButton();
			button->Initialize(&buttonExtent, textParams, border1Params, border2Params);
			SetControlText(button, modOptionConfigs[i].GetName());

			button->AddEvent(CSWGuiControl::AButton, this,
				memberThunkAddr<ModOptions, &ModOptions::onModOption>());
			button->AddEvent(CSWGuiControl::HoverEnter, this,
				memberThunkAddr<ModOptions, &ModOptions::SetDescription>());
			button->SetCustomValue((DWORD)i);

			listButtons.Add(button);
		}

		delete border2Params;
		delete border1Params;
		delete textParams;
		delete protoBorder2;
		delete protoBorder1;
		delete protoText;

		if (listButtons.GetSize() == 0) {
			debugLog("[ModOptions] no mod option configs to show");
			return;
		}

		optionsListBox.AddControls(&listButtons, 1, 0, 0);
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

	void SetDescription(void* control) {
		CSWGuiControl hovered(control);

		size_t index = (size_t)hovered.GetCustomValue();
		if (index >= modOptionConfigs.size()) {
			debugLog("[ModOptions] mod option hovered has out-of-range custom value %u", (unsigned)index);
			return;
		}

		SetControlText(&descriptionLabel, modOptionConfigs[index].GetDescription());

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
};
