#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

// Forward declarations (returned/passed by pointer only)
class CResRef;
class CExoString;
class CSWGuiText;

/// <summary>
/// Standalone helper that wraps a CSWGuiTextParams struct in game memory.
/// Holds the descriptive parameters (text, str ref, font, colors, opacity and
/// the text object) used to configure a CSWGuiText. Does not derive from
/// CSWGuiObject.
/// </summary>
class CSWGuiTextParams : public GameAPIObject {
public:
    explicit CSWGuiTextParams(void* objectPtr);
    CSWGuiTextParams();
    ~CSWGuiTextParams();

    // Accessors (offset without a dedicated game setter)
    float GetOpacity();
    void SetOpacity(float opacity);

    // Accessors (offsets with a dedicated game setter -> getter only).
    // Returned wrappers are heap allocated; caller owns them.
    CExoString* GetText();
    int GetStrRef();
    CResRef* GetFont();
    Vector GetColor();
    Vector GetDefaultColor();
    CSWGuiText* GetTextObject();

    // Functions
    void RestoreDefaultColor();
    void SetAlignment(int alignment);
    void SetBaseFont(CResRef* font);
    void SetColor(Vector* color);
    void SetDefaultColor(Vector* color);
    void SetFont(CResRef* font);
    void SetFontRename();
    void SetStrRef(unsigned long strRef);
    void SetText(CExoString* text);
    void SetTextObject(CSWGuiText* text);

    // Wraps the game's CSWGuiTextParams::operator= (copies rhs into this).
    CSWGuiTextParams& operator=(const CSWGuiTextParams& rhs);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void  (__thiscall* RestoreDefaultColorFn)(void* thisPtr);
    typedef void  (__thiscall* SetAlignmentFn)       (void* thisPtr, int alignment);
    typedef void  (__thiscall* SetBaseFontFn)        (void* thisPtr, void* font);
    typedef void  (__thiscall* SetColorFn)           (void* thisPtr, void* color);
    typedef void  (__thiscall* SetDefaultColorFn)    (void* thisPtr, void* color);
    typedef void  (__thiscall* SetFontFn)            (void* thisPtr, void* font);
    typedef void  (__thiscall* SetFontRenameFn)      (void* thisPtr);
    typedef void  (__thiscall* SetStrRefFn)          (void* thisPtr, unsigned long strRef);
    typedef void  (__thiscall* SetTextFn)            (void* thisPtr, void* text);
    typedef void  (__thiscall* SetTextObjectFn)      (void* thisPtr, void* text);
    typedef void* (__thiscall* AssignFn)             (void* thisPtr, void* rhs);
    typedef void* (__thiscall* ConstructorFn)        (void* thisPtr);

    static RestoreDefaultColorFn restoreDefaultColor;
    static SetAlignmentFn        setAlignment;
    static SetBaseFontFn         setBaseFont;
    static SetColorFn            setColor;
    static SetDefaultColorFn     setDefaultColor;
    static SetFontFn             setFont;
    static SetFontRenameFn       setFontRename;
    static SetStrRefFn           setStrRef;
    static SetTextFn             setText;
    static SetTextObjectFn       setTextObject;
    static AssignFn              assign;
    static ConstructorFn         constructor;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
    static int offsetStrRef;
    static int offsetFont;
    static int offsetColor;
    static int offsetDefaultColor;
    static int offsetTextObject;
    static int offsetOpacity;

    static int classSize;
};
