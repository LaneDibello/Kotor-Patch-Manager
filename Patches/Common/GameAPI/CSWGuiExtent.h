#pragma once

// 16-byte rectangle struct used by every CSWGuiObject (offset "extent").
struct CSWGuiExtent {
    int left;
    int top;
    int width;
    int height;
};
