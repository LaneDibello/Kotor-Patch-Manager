#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"
#include "CSWGuiExtent.h"

class CSWGuiBorder;
class CSWGuiScrollBar;
class CSWGuiControl;
template<typename T> class CExoArrayList;

class CSWGuiListBox : public CSWGuiNavigable {
public:
    explicit CSWGuiListBox(void* objectPtr);
    CSWGuiListBox();
    ~CSWGuiListBox();

    // Accessors
    CSWGuiBorder* GetBorder();
    CSWGuiScrollBar* GetScrollbar();
    CExoArrayList<CSWGuiControl*>* GetControls();
    CExoArrayList<CSWGuiExtent>* GetControlExtents();
    CSWGuiControl* GetHoveredControl();
    CSWGuiControl* GetProtoItem();
    Vector GetColor();
    void SetColor(const Vector& color);
    int GetViewportX();
    int GetViewportY();
    int GetViewportWidth();
    int GetViewportHeight();
    int GetPadding();

    // Functions
    void AddControls(CExoArrayList<CSWGuiControl*>* controls, int suppressCleanUp, int restoreExtents, int varyItemHeights);
    void AddControls(CSWGuiControl** controls, int count, int suppressCleanUp, int restoreExtents, int variableItemHeights);
    void ClearItems();
    int  DisplayToolTip();
    CSWGuiControl* GetControl(int controlId);
    int  GetIsSelectable();
    CSWGuiControl* GetSelectedControl();
    void OrganizeControls();
    void OrganizeOversized();
    void OrganizeUnequal();
    void ReSetFont();
    void SetActive(int active);
    void SetActiveControl(CSWGuiControl* control, int active);
    void SetExtent(CSWGuiExtent* extent);
    void SetPadding(int padding);
    void SetScrollBarExtent(int width, int updateListBoxExtent);
    void SetScrollBarOnLeft(int onLeft);
    void SetSelectedControl(int controlId, int playSound);
    void SetTopVisible(short newTopIndex);
    int  shouldScroll();
    int  getPagesToScroll();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    typedef void  (__thiscall* AddControlsFn)(void* thisPtr, void* controls, int suppressCleanUp, int restoreExtents, int varyItemHeights);
    typedef void  (__thiscall* AddControls2Fn)(void* thisPtr, void* controls, int count, int suppressCleanUp, int restoreExtents, int variableItemHeights);
    typedef void  (__thiscall* ClearItemsFn)(void* thisPtr);
    typedef int   (__thiscall* DisplayToolTipFn)(void* thisPtr);
    typedef void* (__thiscall* GetControlFn)(void* thisPtr, int controlId);
    typedef int   (__thiscall* GetIsSelectableFn)(void* thisPtr);
    typedef void* (__thiscall* GetSelectedControlFn)(void* thisPtr);
    typedef void  (__thiscall* OrganizeControlsFn)(void* thisPtr);
    typedef void  (__thiscall* OrganizeOversizedFn)(void* thisPtr);
    typedef void  (__thiscall* OrganizeUnequalFn)(void* thisPtr);
    typedef void  (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void  (__thiscall* SetActiveFn)(void* thisPtr, int active);
    typedef void  (__thiscall* SetActiveControlFn)(void* thisPtr, void* control, int active);
    typedef void  (__thiscall* SetExtentFn)(void* thisPtr, void* extent);
    typedef void  (__thiscall* SetPaddingFn)(void* thisPtr, int padding);
    typedef void  (__thiscall* SetScrollBarExtentFn)(void* thisPtr, int width, int updateListBoxExtent);
    typedef void  (__thiscall* SetScrollBarOnLeftFn)(void* thisPtr, int onLeft);
    typedef void  (__thiscall* SetSelectedControlFn)(void* thisPtr, int controlId, int playSound);
    typedef void  (__thiscall* SetTopVisibleFn)(void* thisPtr, short newTopIndex);
    typedef int   (__thiscall* ShouldScrollFn)(void* thisPtr);
    typedef int   (__thiscall* GetPagesToScrollFn)(void* thisPtr);

    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static AddControlsFn        addControls;
    static AddControls2Fn       addControls2;
    static ClearItemsFn         clearItems;
    static DisplayToolTipFn     displayToolTip;
    static GetControlFn         getControl;
    static GetIsSelectableFn    getIsSelectable;
    static GetSelectedControlFn getSelectedControl;
    static OrganizeControlsFn   organizeControls;
    static OrganizeOversizedFn  organizeOversized;
    static OrganizeUnequalFn    organizeUnequal;
    static ReSetFontFn          reSetFont;
    static SetActiveFn          setActive;
    static SetActiveControlFn   setActiveControl;
    static SetExtentFn          setExtent;
    static SetPaddingFn         setPadding;
    static SetScrollBarExtentFn setScrollBarExtent;
    static SetScrollBarOnLeftFn setScrollBarOnLeft;
    static SetSelectedControlFn setSelectedControl;
    static SetTopVisibleFn      setTopVisible;
    static ShouldScrollFn       shouldScrollFn;
    static GetPagesToScrollFn   getPagesToScrollFn;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetBorder;
    static int offsetScrollbar;
    static int offsetControls;
    static int offsetControlExtents;
    static int offsetHoveredControl;
    static int offsetProtoItem;
    static int offsetColor;
    static int offsetViewportX;
    static int offsetViewportY;
    static int offsetViewportWidth;
    static int offsetViewportHeight;
    static int offsetPadding;
};
