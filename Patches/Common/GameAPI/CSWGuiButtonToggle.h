#pragma once
#include "../Common.h"
#include "CSWGuiButton.h"

class CSWGuiButtonToggle : public CSWGuiButton {
public:
    explicit CSWGuiButtonToggle(void* objectPtr);
    ~CSWGuiButtonToggle();

    // Functions
    void SetSelected(UINT selected);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* SetSelectedFn)(void* thisPtr, UINT selected);

    static SetSelectedFn setSelected;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
