#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiControl;
class CSWGuiBorder;
template<typename T> class CExoArrayList;

class CSWGuiPanel : public CSWGuiObject {
public:
    explicit CSWGuiPanel(void* objectPtr);
    ~CSWGuiPanel();

    // Accessors
    CSWGuiControl* GetActiveControl();
    CExoArrayList<CSWGuiControl*>* GetControls();
    float GetAlpha();
    void SetAlpha(float alpha);
    Vector GetColor();
    void SetColor(const Vector& color);
    CSWGuiBorder* GetBorder();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetActiveControl;
    static int offsetControls;
    static int offsetAlpha;
    static int offsetColor;
    static int offsetBorder;
};
