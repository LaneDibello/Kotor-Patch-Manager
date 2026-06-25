#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiControl;
class CSWGuiBorder;
class CSWGuiManager;
class CExoString;
class CResRef;
template<typename T> class CExoArrayList;

class CSWGuiPanel : public CSWGuiObject {
public:
    explicit CSWGuiPanel(void* objectPtr);
    // Allocates a new panel and runs the game's CSWGuiPanel(CSWGuiManager*) constructor.
    explicit CSWGuiPanel(CSWGuiManager* manager);
    ~CSWGuiPanel();

    // Accessors
    CSWGuiControl* GetActiveControl();
    CExoArrayList<CSWGuiControl*>* GetControls();
    float GetAlpha();
    void SetAlpha(float alpha);
    Vector GetColor();
    void SetColor(const Vector& color);
    CSWGuiBorder* GetBorder();

    // Functions
    void AddControl(CSWGuiControl* control);
    void CenterPanel();
    CSWGuiControl* GetControl(int index);
    void GetExtentAccountingForPanelOffset(CSWGuiExtent* outExtent);
    void GetFullScreenBG(CExoString* outBGString);
    void GetLocalMouseCoords(int* outX, int* outY);
    bool HitCheckMouse(int mouseX, int mouseY);
    void InitControl(CSWGuiControl* controlToInit, CExoString* label, int activate);
    void ResetFont();
    void SetActiveControl(CSWGuiControl* controlToActivate, int playSound);
    void SetBackground(CResRef* image);
    void SetVisible(int isVisible);
    void StartLoadFromLayout(CResRef* guiResref);
    void StopLoadFromLayout();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void  (__thiscall* AddControlFn)(void* thisPtr, void* control);
    typedef void  (__thiscall* CenterPanelFn)(void* thisPtr);
    typedef void* (__thiscall* GetControlFn)(void* thisPtr, int index);
    typedef void  (__thiscall* GetExtentAccountingForPanelOffsetFn)(void* thisPtr, CSWGuiExtent* outExtent);
    typedef void  (__thiscall* GetFullScreenBGFn)(void* thisPtr, void* outBGString);
    typedef void  (__thiscall* GetLocalMouseCoordsFn)(void* thisPtr, int* outX, int* outY);
    typedef bool  (__thiscall* HitCheckMouseFn)(void* thisPtr, int mouseX, int mouseY);
    typedef void  (__thiscall* InitControlFn)(void* thisPtr, void* controlToInit, void* label, int activate);
    typedef void  (__thiscall* ResetFontFn)(void* thisPtr);
    typedef void  (__thiscall* SetActiveControlFn)(void* thisPtr, void* controlToActivate, int playSound);
    typedef void  (__thiscall* SetBackgroundFn)(void* thisPtr, void* image);
    typedef void  (__thiscall* SetVisibleFn)(void* thisPtr, int isVisible);
    typedef void  (__thiscall* StartLoadFromLayoutFn)(void* thisPtr, void* guiResref);
    typedef void  (__thiscall* StopLoadFromLayoutFn)(void* thisPtr);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr, void* manager);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static AddControlFn addControl;
    static CenterPanelFn centerPanel;
    static GetControlFn getControl;
    static GetExtentAccountingForPanelOffsetFn getExtentAccountingForPanelOffset;
    static GetFullScreenBGFn getFullScreenBG;
    static GetLocalMouseCoordsFn getLocalMouseCoords;
    static HitCheckMouseFn hitCheckMouse;
    static InitControlFn initControl;
    static ResetFontFn resetFont;
    static SetActiveControlFn setActiveControl;
    static SetBackgroundFn setBackground;
    static SetVisibleFn setVisible;
    static StartLoadFromLayoutFn startLoadFromLayout;
    static StopLoadFromLayoutFn stopLoadFromLayout;
    static ConstructorFn constructor;
    static DestructorFn  destructor;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetActiveControl;
    static int offsetControls;
    static int offsetAlpha;
    static int offsetColor;
    static int offsetBorder;

    static int classSize;
};
