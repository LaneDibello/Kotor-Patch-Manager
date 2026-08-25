#pragma once
#include "Common.h"
#include "ModOptionsConfig.h"

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
#include <map>
#include <string>
#include <vector>

class ModOptions : public CSWGuiPanel {
public:
	//Controls
	CSWGuiLabel titleLabel;
	CSWGuiListBox optionsListBox;
	CSWGuiLabel descriptionLabel;
	CSWGuiListBox descriptionListBox;
	CSWGuiButton backButton;
	CSWGuiButton refreshButton;

	std::map<int, std::string> modOptionConfigs;

	// Callbacks
	void onModOption(void* control) {
		debugLog("Pressed Button at %X", control);

		CSWGuiControl button(control);
		std::string configPath = modOptionConfigs[button.GetId()];

		
		// Create new OptionsMenu with this config

		
	}
	void onRefresh(void* control) {
		debugLog("Pressed Button at %X", control);
		populateOptionsListBox();
	}
	void onBack(void* control) {
		debugLog("Pressed Button at %X", control);
		// Should return to the options menu
		// Perhaps via using the modal stack
		// To push a modal to the stack by using flags
		// that are `& 1` on the AddPanel method
	}

	ModOptions(CSWGuiManager* manager) :
		CSWGuiPanel(manager),
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

		// Figure out how to handle the Description ListBox/Label
		// Looks like `CSWGuiLabel::Initialize` (0xC stack varient)
		// Is the way to go for getting the protoitem set up. 
		// See also: CSWGuiOptionsFeedback::SetDescription

		refreshButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&ModOptions::onRefresh));
		backButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&ModOptions::onBack));

		this->OverrideHandleInputEvent(memberFuncAddr(&ModOptions::_HandleInputEvent));
	}

private:
	void populateOptionsListBox() {
		std::vector<std::string> tomls;
		std::vector<std::string> optionsNames;

		std::filesystem::path directory("Mod Options");
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".toml") {
				continue;
			}

			std::string path = entry.path().string();
			std::string menuName;
			if (!ModOptionsConfig::ReadMenuName(path, menuName)) {
				continue;
			}

			tomls.push_back(path);
			optionsNames.push_back(menuName);
		}

		CSWGuiButton proto(optionsListBox.GetProtoItem()->GetPtr());
		CSWGuiExtent buttonExtent;
		buttonExtent.top = 0;
		buttonExtent.left = 0;
		buttonExtent.width = testListBox.GetViewportWidth() - 2 * testListBox.GetPadding();
		buttonExtent.height = proto.GetExtent().height;

		CExoArrayList<CSWGuiControl*> listButtons;
		for (size_t i = 0; i < optionsNames.size(); ++i) {
			CSWGuiButton* button = new CSWGuiButton();
			button->Initialize(&buttonExtent,
				proto.GetText()->GetTextParams(),
				proto.GetBorder1()->GetBorderParams(),
				proto.GetBorder2()->GetBorderParams());
			CExoString buttonText(optionsNames.at(i).c_str());
			button->GetText()->GetTextParams()->SetText(&buttonText);
			button->AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&ModOptions::onModOption));
			listButtons.Add(button);
			
			modOptionConfigs[button->GetId()] = tomls.at(i);
		}
		optionsListBox.AddControls(&listButtons, 1, 0, 0);
		testListBox.SetSelectedControl(0, 0);
	}

	void _HandleInputEvent(int event, int doPanelEvents) {
		if (doPanelEvents) {
			switch (event) {
			case CSWGuiControl::AButton:
				// Activate the currently selected button
				// Might not need to do anything here
				break;
			case CSWGuiControl::BButton:
				// Perform the "Back" behavior
				break;
			default:
				break;
			}
		}

		HandleInputEvent(event, doPanelEvents);
	}

	void SetDescription(void* control) {
		CSWGuiControl hovered(control);


	}
};