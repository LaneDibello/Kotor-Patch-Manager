#include "Common.h"

#include "GameAPI/Camera.h"
#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWGui3DSceneView.h"
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
		// Should open the Corresponding Mod Options menu

		CSWGuiControl button(control);
		std::string config = modOptionConfigs[button.GetId()];

		// Load the TOML file, and build the Options Menu
	}
	void onRefresh(void* control) {
		debugLog("Pressed Button at %X", control);
		populateOptionsListBox()
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
		// TODO: Read from the TOMLs in the "Mod Options"
		// directory for each add a button that will link 
		// to that custom options menu.
		std::string directory("Mod Options");

		// tomls should be a list of every toml in directory
		std::vector<std::string> tomls;
		// optionsNames is the `name` filed from each TOML
		std::vector<std::string> optionsNames;

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
				break;
			case CSWGuiControl::BButton:
				// Perform the "Back" behavior
				break;
			default:
				break;
			}
		}

		HandleInputEvent(event, param2);
	}
};