#pragma once
#include "../Common.h"

// Free-standing Aurora GUI render-context functions. CSWGuiPanel::Draw brackets
// its border and controls with these; anything drawn outside a layer/viewport
// pair never reaches the screen.
namespace AurGUI {
    void StartLayer();
    void StopLayer();
    // Returns nonzero when the viewport opened; only then draw and call CloseViewport.
    int SetupViewport(int left, int top, int width, int height, Vector* color, bool param6, float alpha);
    void CloseViewport();
}
