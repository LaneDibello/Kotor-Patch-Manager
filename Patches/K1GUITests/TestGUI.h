#include "Common.h"
#include "GameAPI/CSWGuiPanel.h"
#include "GameAPI/CSWGuiManager.h"
#include "GameAPI/CSWGuiLabel.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiText.h"
#include "GameAPI/CSWGuiTextParams.h"
#include "GameAPI/CSWGuiBorder.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiListBox.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CExoArrayList.h"
#include "GameAPI/CSWGui3DSceneView.h"
#include "GameAPI/CSWGuiScene.h"
#include "GameAPI/Gob.h"
#include "GameAPI/Camera.h"
#include "GameAPI/Scene.h"

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
    CSWGuiLabel imageLabel;
    CSWGui3DSceneView scene3d;

    // Callbacks
    void buttonCallback(void* control) {
        debugLog("Pressed Button at %X", control);
    }

    // Custom input handler, registered via OverrideInputEvent below. Runs with
    // this TestGUI as its `this`, so it can reach our controls; calls the wrapper's
    // HandleInputEvent to invoke the game's original behavior.
    void _HandleInputEvent(int event, int param2) {
        debugLog("TestGUI _HandleInputEvent: this=%X event=%d param2=%d", this, event, param2);
        if (param2) {
            Camera* camera = scene3d.GetScene()->GetCamera();
            Vector scrap;
            Vector* position = camera->GetPosition(&scrap);
            Vector newPosition = *position;
            debugLog("Old Position: %f, %f, %f", position->x, position->y, position->z);
            switch (event) {
            case CSWGuiControl::UpArrow:
                newPosition.y = newPosition.y + 0.5;
                break;

            case CSWGuiControl::DownArrow:
                newPosition.y = newPosition.y - 0.5;
                break;

            case CSWGuiControl::LeftArrow:
                newPosition.x = newPosition.x - 0.5;
                break;

            case CSWGuiControl::RightArrow:
                newPosition.x = newPosition.x + 0.5;
                break;

            case CSWGuiControl::MenuRight:
                newPosition.z = newPosition.z + 0.5;
                break;

            case CSWGuiControl::MenuLeft:
                newPosition.z = newPosition.z - 0.5;
                break;
            default:
                break;

            }
            debugLog("New Position: %f, %f, %f", newPosition.x, newPosition.y, newPosition.z);
            camera->SetPosition(&scrap, newPosition);
        }

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
        testListBox(),
        imageLabel(),
        scene3d()
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
        CExoString lbMonk("L_MONK");
        this->InitControl(&imageLabel, &lbMonk, 1);
        CExoString test3DTag("TEST_3D");
        this->InitControl(&scene3d, &test3DTag, 1);
        this->StopLoadFromLayout();

        debugLog("Load from layout finished");

        CSWGuiScene * guiScene = scene3d.GetScene();
        Vector zeroV{0.0f, 0.0f, 0.0f};
        Quaternion quat{ 0.0f, 0.0f, 0.0f, 1.0f };
        guiScene->GetScene()->SpawnRoom("gui3D_room", &zeroV, &quat);

        debugLog("Room Spawned");

        //CExoString model("upgitem_light");
        //CExoString model("shaolin_test");
        //Gob* gameObject = guiScene->AddModel(&model, -1);
        CExoString model2("gidy_sun");
        guiScene->AddModel(&model2, -1);
        CExoString model3("c_kinrath");
        Gob* gameObject = guiScene->AddModel(&model3, -1);

        debugLog("Model added to Scene. Gob at %X", gameObject);

        Camera * camera = scene3d.GetScene()->GetCamera();
        //camera->AttachToObject(gameObject, "camerahook", 0);
        //camera->AttachToObject(nullptr, nullptr, 0);
        Vector scrap;
        Vector pos{ 0.0f, 0.0f, 3.5f };
        camera->SetPosition(&scrap, pos);
        Quaternion scrap2;
        Quaternion orientation{ 0.0f, 0.0f, 0.0f, 1.0f };
        camera->SetOrientation(&scrap2, orientation);


        debugLog("scene3d is at %X, Camera is at %X, Game object at %X", &scene3d, camera, gameObject);

        // Style each item from the listbox's proto item (loaded from the .gui).
        // Width comes from the viewport (minus padding), height from the proto extent.
        CSWGuiButton proto(testListBox.GetProtoItem()->GetPtr());
        CSWGuiExtent buttonExtent;
        buttonExtent.top = 0;
        buttonExtent.left = 0;
        buttonExtent.width = testListBox.GetViewportWidth() - 2 * testListBox.GetPadding();
        buttonExtent.height = proto.GetExtent().height;

        CExoArrayList<CSWGuiControl*> listButtons;
        for (int i = 0; i < 5; i++) {
            CSWGuiButton* button = new CSWGuiButton();
            button->Initialize(&buttonExtent,
                               proto.GetText()->GetTextParams(),
                               proto.GetBorder1()->GetBorderParams(),
                               proto.GetBorder2()->GetBorderParams());
            char testBuffer[16];
            sprintf_s(testBuffer, 16, "Button %i", i);
            CExoString buttonText(testBuffer);
            button->GetText()->GetTextParams()->SetText(&buttonText);
            button->AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback)); //OnClick Event
            listButtons.Add(button);
        }
        testListBox.AddControls(&listButtons, 1, 0, 0);
        testListBox.SetSelectedControl(0, 0);

        redButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback));
        orangeButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback));
        yellowButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback));
        greenButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback));
        blueButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback));
        violetButton.AddEvent(CSWGuiControl::AButton, this, memberFuncAddr(&TestGUI::buttonCallback));

        // Redirect the panel's HandleInputEvent to our handler
        this->OverrideHandleInputEvent(memberFuncAddr(&TestGUI::_HandleInputEvent));
        this->OverrideOnPanelAdded(memberFuncAddr(&TestGUI::_OnPanelAdded));
        this->OverrideOnPanelRemoved(memberFuncAddr(&TestGUI::_OnPanelRemoved));
        //this->OverrideDraw(memberFuncAddr(&TestGUI::_Draw));
        //this->OverrideUpdate(memberFuncAddr(&TestGUI::_Update));
	}

};