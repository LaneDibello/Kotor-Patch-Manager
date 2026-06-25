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
    Vector GetColor();
    void SetColor(const Vector& color);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetBorder;
    static int offsetScrollbar;
    static int offsetControls;
    static int offsetControlExtents;
    static int offsetHoveredControl;
    static int offsetColor;
};
