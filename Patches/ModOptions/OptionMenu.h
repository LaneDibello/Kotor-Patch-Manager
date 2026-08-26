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
#include "GameAPI/CSWGuiButtonToggle.h"
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

	//Callbacks
	void onBack(void* control) {
		debugLog("[ModOptions] Back Button Pressed");
		_HandleInputEvent(CSWGuiControl::BButton, 1);
	}

	void onDefault(void* control) {
		debugLog("[ModOptions] Default Button Pressed");
		// Restore all options to their default states
	}

	void onOption()(void* control) {
		// Pull the option from config
		// Get the state of the control
		// Do the ini work if necessary
		// Run the function if it exists
	}

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
		CSWGuiLabel* descProto = new CSWGuiLabel(descriptionListBox.GetProtoItem()->GetPtr());
		descriptionLabel.Initialize(descProto->GetExtent(), descProto, 1.0f);

		defaultButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&OptionsMenu::onDefault));
		backButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&OptionsMenu::onBack));

	}
	// TODO: Implement me
private:
	void populateOptionsListBox() {
		if (!config || !config->loaded) {
			return;
		}

		optionsListBox.ClearItems();

		CSWGuiButton proto(optionsListBox.GetProtoItem()->GetPtr());
		CSWGuiExtent optionExtent;
		optionExtent.top = 0;
		optionExtent.left = 0;
		optionExtent.width = optionsListBox.GetViewportWidth() - 2 * optionsListBox.GetPadding();
		optionExtent.height = proto.GetExtent().height;

		CExoArrayList<CSWGuiControl*> listOptions;
		for (const ModOption& option : config->GetOptions()) {
			switch (option.type) {
			case ModOptionType::Toggle:
				CSWGuiButtonToggle* toggle = new CSWGuiButtonToggle();
				toggle->Initialize(&optionExtent,
					proto.GetText()->GetTextParams(),
					proto.GetBorder1()->GetBorderParams(),
					proto.GetBorder2()->GetBorderParams());
				//todo
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
};