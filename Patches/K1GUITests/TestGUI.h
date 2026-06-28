#include "Common.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CExoString.h"

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
        violetButton()
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
        this->StopLoadFromLayout();

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
        this->OverrideDraw(memberFuncAddr(&TestGUI::_Draw));
        this->OverrideUpdate(memberFuncAddr(&TestGUI::_Update));
	}

};