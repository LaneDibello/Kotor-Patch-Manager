#include "Common.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"
#include "GameAPI/CSWGuiListBox.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CExoArrayList.h"

class TestGUI : public CSWGuiPanel {
public:
    //Controls
    CSWGuiLabel titleLabel;
    CSWGuiLabel testLabel;
    CSWGuiButton redButton;
    CSWGuiButton orangeButton;
    CSWGuiButton yellowButton;
    CSWGuiButton greenButton;
    CSWGuiButton blueButton;
    CSWGuiButton violetButton;
    CSWGuiListBox testListBox;

    // Callbacks
    void buttonCallback(void* control) {
        debugLog("Pressed Button at %X", control);
    }

    // Custom input handler, registered via OverrideInputEvent below. Runs with
    // this TestGUI as its `this`, so it can reach our controls; calls the wrapper's
    // HandleInputEvent to invoke the game's original behavior.
    void _HandleInputEvent(int event, int param2) {
        debugLog("TestGUI _HandleInputEvent: this=%X event=%d param2=%d", this, event, param2);
        HandleInputEvent(event, param2);
    }

    void _OnPanelAdded() {
        debugLog("TestGUI _OnPanelAdded: this=%X", this);
        OnPanelAdded();
    }

    void _OnPanelRemoved() {
        debugLog("TestGUI _OnPanelRemoved: this=%X", this);
        OnPanelRemoved();
    }

    void _Draw(float f) {
        debugLog("TestGUI _Draw: this=%X param=%f", this, f);
        Draw(f);
    }

    void _Update(float f) {
        debugLog("TestGUI _Update: this=%X param=%f", this, f);
    }

	TestGUI(CSWGuiManager* manager) :
        CSWGuiPanel(manager),
        titleLabel(),
        testLabel(),
        redButton(),
        orangeButton(),
        yellowButton(),
        greenButton(),
        blueButton(),
        violetButton(),
        testListBox()
    {

        CResRef guiResref("test1");
        this->StartLoadFromLayout(&guiResref);
        CExoString titleTag("LBL_TITLE");
        this->InitControl(&titleLabel, &titleTag, 1);
        CExoString testTag("LBL_TEST");
        this->InitControl(&testLabel, &testTag, 1);
        CExoString redTag("B_RED");
        this->InitControl(&redButton, &redTag, 1);
        CExoString orangeTag("B_ORANGE");
        this->InitControl(&orangeButton, &orangeTag, 1);
        CExoString yellowTag("B_YELLOW");
        this->InitControl(&yellowButton, &yellowTag, 1);
        CExoString greenTag("B_GREEN");
        this->InitControl(&greenButton, &greenTag, 1);
        CExoString blueTag("B_BLUE");
        this->InitControl(&blueButton, &blueTag, 1);
        CExoString violetTag("B_VIOLET");
        this->InitControl(&violetButton, &violetTag, 1);
        CExoString lbTag("LB_TEST");
        this->InitControl(&testListBox, &lbTag, 1);
        this->StopLoadFromLayout();

        CSWGuiExtent buttonExtent;
        buttonExtent.top = 0;
        buttonExtent.left = 0;
        buttonExtent.height = 100;
        buttonExtent.width = 100;

        CExoArrayList<CSWGuiControl*> listButtons;
        for (int i = 0; i < 5; i++) {
            CSWGuiButton* button = new CSWGuiButton();
            button->SetExtent(buttonExtent);
            char testBuffer[16];
            sprintf_s(testBuffer, 16, "Button %i", i);
            CExoString buttonText(testBuffer);
            button->GetText()->GetTextParams()->SetText(&buttonText);
            listButtons.Add(button);
        }
        testListBox.AddControls(&listButtons, 0, 1, 0);
        testListBox.SetSelectedControl(0, 0);

        redButton.AddEvent(0x27, this, memberFuncAddr(&TestGUI::buttonCallback));
        orangeButton.AddEvent(0x27, this, memberFuncAddr(&TestGUI::buttonCallback));
        yellowButton.AddEvent(0x27, this, memberFuncAddr(&TestGUI::buttonCallback));
        greenButton.AddEvent(0x27, this, memberFuncAddr(&TestGUI::buttonCallback));
        blueButton.AddEvent(0x27, this, memberFuncAddr(&TestGUI::buttonCallback));
        violetButton.AddEvent(0x27, this, memberFuncAddr(&TestGUI::buttonCallback));

        // Redirect the panel's HandleInputEvent to our handler
        this->OverrideHandleInputEvent(memberFuncAddr(&TestGUI::_HandleInputEvent));
        this->OverrideOnPanelAdded(memberFuncAddr(&TestGUI::_OnPanelAdded));
        this->OverrideOnPanelRemoved(memberFuncAddr(&TestGUI::_OnPanelRemoved));
        //this->OverrideDraw(memberFuncAddr(&TestGUI::_Draw));
        //this->OverrideUpdate(memberFuncAddr(&TestGUI::_Update));
	}

};