#pragma once
#include "../Common.h"
#include "../VTableOverride.h"
#include "CSWGuiObject.h"

class CSWGuiControl;
class CSWGuiBorder;
class CSWGuiManager;
class CExoString;
class CResRef;
template<typename T> class CExoArrayList;

// CSWGuiPanel virtual-function table layout for KotOR 1 (Windows): 27 entries /
// 108 bytes. The order and membership of vtable slots varies between game
// versions and platforms (e.g. the Mac builds duplicate the destructor slot, and
// KotOR 2 adds entries), so this enum is specific to K1/Windows. Additional
// layouts are added behind CSWGuiPanel::PanelVTableCount() as they are supported.
// Used to index a copied vtable when overriding panel virtuals (see VTableOverride.h).
enum class PanelVTableSlot : int {
    Destructor = 0,
    SetExtent,
    SetActiveControl,
    HandleMouseOver,
    HandleMouseCapturedMovement,
    HandleLoseMouseFocus,
    HandleLMouseDown,
    HandleLMouseUp,
    HandleRMouseDown,
    HandleRMouseUp,
    AsSWGuiPanel,
    AsSWGuiListBox,
    AsSWGuiControl,
    Draw,
    Update,
    HandleInputEvent,
    HitCheckMouse,
    GetActiveControl,
    OnPanelAdded,
    OnPanelRemoved,
    OnAButtonPressed,
    OnBButtonPressed,
    OnXButtonPressed,
    OnYButtonPressed,
    OnBlackButtonPressed,
    GetFullScreenBG,
    ResetFont
};

// Number of function entries in the K1/Windows panel vtable above.
inline constexpr int PANEL_VTABLE_SLOT_COUNT = 27;

class CSWGuiPanel : public CSWGuiObject {
public:
    explicit CSWGuiPanel(void* objectPtr);
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
    void HandleInputEvent(int event, int param2);
    void Draw(float param1);
    void OnPanelAdded();
    void OnPanelRemoved();
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

    // Redirect the panel's HandleInputEvent virtual to a method on the derived
    // wrapper. Pass it through memberFuncAddr (see Common.h).
    // The handler runs with this wrapper as its `this`, so it can reach the
    // wrapper's controls, and may call HandleInputEvent() to invoke the original.
    void OverrideHandleInputEvent(void* handler);

    // Redirect the panel's Draw virtual
    void OverrideDraw(void* handler);

    // Redirect OnPanelAdded / OnPanelRemoved 
    void OverrideOnPanelAdded(void* handler);
    void OverrideOnPanelRemoved(void* handler);

    // Redirect the panel's Update virtual
    void OverrideUpdate(void* handler);

    // Returns the number of entries in the game's CSWGuiPanel vtable for the
    // currently-detected game version, or -1 if the version is unsupported (in
    // which case vtable overriding is disabled). This is the single place new
    // game versions/platforms are wired in.
    static int PanelVTableCount();

protected:
    typedef void  (__thiscall* AddControlFn)(void* thisPtr, void* control);
    typedef void  (__thiscall* CenterPanelFn)(void* thisPtr);
    typedef void  (__thiscall* HandleInputEventFn)(void* thisPtr, int event, int param2);
    typedef void  (__thiscall* DrawFn)(void* thisPtr, float param1);
    typedef void  (__thiscall* OnPanelAddedFn)(void* thisPtr);
    typedef void  (__thiscall* OnPanelRemovedFn)(void* thisPtr);
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
    static HandleInputEventFn handleInputEvent;
    static DrawFn draw;
    static OnPanelAddedFn onPanelAdded;
    static OnPanelRemovedFn onPanelRemoved;
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

    // Per-instance vtable override (null until the first Override* call). Owned;
    // destroyed before the game object so its original vtable is restored first.
    VTableOverride* vtableOverride = nullptr;

    // Raw addresses of the derived wrapper's handlers (via memberFuncAddr), invoked
    // by the matching thunk. Null when that override is not registered.
    void* inputEventHandler = nullptr;
    void* drawHandler = nullptr;
    void* onPanelAddedHandler = nullptr;
    void* onPanelRemovedHandler = nullptr;
    void* updateHandler = nullptr;

    // Lazily creates the per-instance vtable override on first use. Returns false
    // if the game version's panel vtable layout is unsupported.
    bool EnsureVTableOverride();

    // Installed into the matching vtable slot. The game calls these as __thiscall
    // (game object in ECX); we recover the owning wrapper from the override's
    // back-pointer and forward to its registered handler. __fastcall stands in for
    // __thiscall on these free-standing functions (MSVC forbids __thiscall here);
    // the static members can still touch private state.
    static void __fastcall HandleInputEventThunk(void* gameObj, void* edx, int event, int param2);
    static void __fastcall DrawThunk(void* gameObj, void* edx, float param1);
    static void __fastcall OnPanelAddedThunk(void* gameObj, void* edx);
    static void __fastcall OnPanelRemovedThunk(void* gameObj, void* edx);
    static void __fastcall UpdateThunk(void* gameObj, void* edx, float param1);
};
