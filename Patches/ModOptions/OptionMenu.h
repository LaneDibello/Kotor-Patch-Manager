// TODO: Draw up class for individual mod option menus
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

#include <string>
#include <vector>

class OptionsMenu : public CSWGuiPanel {
public:
	// Non-owning; ModOptions owns the config objects.
	ModOptionsConfig* config;

	CSWGuiLabel titleLabel;
	CSWGuiListBox optionsListBox;
	CSWGuiLabel descriptionLabel;
	CSWGuiListBox descriptionListBox;
	CSWGuiButton backButton;
	CSWGuiButton defaultButton;

	OptionsMenu(CSWGuiManager* manager, ModOptionsConfig* menuConfig) :
		CSWGuiPanel(manager),
		config(menuConfig),
		titleLabel(),
		optionsListBox(),
		descriptionLabel(),
		descriptionListBox(),
		backButton(),
		defaultButton()
	{
		if (!config || !config->loaded) {
			debugLog("[ModOptions] OptionsMenu built without a loaded config\n");
			return;
		}

		CResRef guiResref("modOptionMenu");
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

		populateOptionsListBox();

		//Description Logic


	}
	// TODO: Implement me
private:
	void populateOptionsListBox() {
		// Fill out the options with controls from the config
		if (!config || !config->loaded) {
			return;
		}

		for (const ModOption& option : config->GetOptions()) {
			switch (option.type) {
			case ModOptionType::Toggle:
				break;
			case ModOptionType::Slider:
				break;
			case ModOptionType::List:
				break;
			case ModOptionType::Text:
				break;
			}
		}
	}
};