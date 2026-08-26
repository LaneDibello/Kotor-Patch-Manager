#pragma once
#include "Common.h"
#include "ModOptionsConfig.h"
#include "OptionsMenu.h"

#include "GameAPI/Camera.h"
#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiListBox.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiScene.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"
#include "GameAPI/Gob.h"
#include "GameAPI/Scene.h"

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

	// Index coresponds to an option button's custom_value
	std::vector<ModOptionsConfig*> modOptionConfigs;

	// Callbacks
	void onModOption(void* control) {
		CSWGuiControl button(control);
		size_t index = (size_t)button.GetCustomValue();
		if (index >= modOptionConfigs.size()) {
			debugLog("[ModOptions] mod option button has out-of-range custom value %u", (unsigned)index);
			return;
		}

		ModOptionsConfig* config = modOptionConfigs[index];
		debugLog("[ModOptions] selected `%s` (%s)", config->GetName().c_str(), config->GetSourcePath().c_str());

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
		CResRef guiResref("modOptions");
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

		populateOptionsListBox();

		// Set up description ListBox
		CSWGuiLabel* descProto = new CSWGuiLabel(descriptionListBox.GetProtoItem()->GetPtr());
		descriptionLabel.Initialize(descProto->GetExtent(), descProto, 1.0f);

		refreshButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&ModOptions::onRefresh));
		backButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&ModOptions::onBack));

		this->OverrideHandleInputEvent(memberFuncAddr(&ModOptions::_HandleInputEvent));
	}

	~ModOptions() {
		clearModOptionConfigs();
	}

private:
	void clearModOptionConfigs() {
		for (ModOptionsConfig* config : modOptionConfigs) {
			delete config;
		}
		modOptionConfigs.clear();
	}

	void loadModOptionConfigs() {
		clearModOptionConfigs();

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

			modOptionConfigs.push_back(new ModOptionsConfig(std::move(config)));
		}

		debugLog("[ModOptions] loaded %u mod option config(s)", (unsigned)modOptionConfigs.size());
	}
	void populateOptionsListBox() {
		loadModOptionConfigs();

		optionsListBox.ClearItems();

		CSWGuiButton proto(optionsListBox.GetProtoItem()->GetPtr());
		CSWGuiExtent buttonExtent;
		buttonExtent.top = 0;
		buttonExtent.left = 0;
		buttonExtent.width = optionsListBox.GetViewportWidth() - 2 * optionsListBox.GetPadding();
		buttonExtent.height = proto.GetExtent().height;

		CExoArrayList<CSWGuiControl*> listButtons;
		for (size_t i = 0; i < modOptionConfigs.size(); ++i) {
			CSWGuiButton* button = new CSWGuiButton();
			button->Initialize(&buttonExtent,
				proto.GetText()->GetTextParams(),
				proto.GetBorder1()->GetBorderParams(),
				proto.GetBorder2()->GetBorderParams());
			CExoString buttonText(const_cast<char*>(modOptionConfigs[i]->GetName().c_str()));
			button->GetText()->GetTextParams()->SetText(&buttonText);
			button->AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&ModOptions::onModOption));
			button->AddEvent(CSWGuiControl::HoverEnter, this, memberFuncAddr(&ModOptions::SetDescription));
			button->SetCustomValue((DWORD)i);

			listButtons.Add(button);
		}
		optionsListBox.AddControls(&listButtons, 1, 0, 0);
		optionsListBox.SetSelectedControl(0, 0);
	}

	void _HandleInputEvent(int event, int doPanelEvents) {
		if (doPanelEvents) {
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
		ModOptionsConfig* config = modOptionConfigs[index];

		CExoString description(const_cast<char*>(config->GetDescription().c_str()));

		descriptionListBox.ClearItems();
		CSWGuiExtent lbExtent = descriptionListBox.GetExtent();
		lbExtent.height = descriptionLabel.GetText()->GetIdealHeight();

		descriptionLabel.SetExtent(lbExtent);

		CSWGuiControl* descPtr = &descriptionLabel;

		descriptionListBox.AddControls(&descPtr, 1, 0, 0, 0);
		descriptionListBox.SetActiveControl(&descriptionLabel, 0);
	}
};