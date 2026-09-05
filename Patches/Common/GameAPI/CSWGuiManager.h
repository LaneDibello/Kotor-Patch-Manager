#pragma once
#include "../Common.h"
#include "GameAPIObject.h"
#include "CExoArrayList.h"

class CSWGuiPanel;

/// <summary>
/// Wraps the game's CSWGuiManager, a top-level singleton tracked by the
/// GUI_MANAGER_PTR global pointer. The default constructor retrieves and wraps
/// that instance automatically. Does not derive from CSWGuiObject.
/// </summary>
class CSWGuiManager : public GameAPIObject {
public:
    // Wraps an explicit CSWGuiManager pointer.
    explicit CSWGuiManager(void* objectPtr);
    // Automatically retrieves and wraps the global CSWGuiManager (GUI_MANAGER_PTR).
    CSWGuiManager();
    ~CSWGuiManager();

    // Accessors
    short GetViewportWidth();
    void SetViewportWidth(short width);
    short GetViewportHeight();
    void SetViewportHeight(short height);

    // Returned wrapper is heap allocated; caller owns it.
    CExoArrayList<CSWGuiPanel*>* GetPanels();

    // Functions
    void AddPanel(CSWGuiPanel* panel, int flags, int playSound);
    // Plays one of the manager's preloaded GUI sounds by index.
    void PlayGuiSound(byte soundId);
    // Pops the top panel off the modal stack; returns the new stack depth.
    int PopModalPanel();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* AddPanelFn)(void* thisPtr, void* panel, int flags, int playSound);
    typedef void (__thiscall* PlayGuiSoundFn)(void* thisPtr, byte soundId);
    typedef int (__thiscall* PopModalPanelFn)(void* thisPtr);

    static AddPanelFn addPanel;
    static PlayGuiSoundFn playGuiSound;
    static PopModalPanelFn popModalPanel;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetViewportWidth;
    static int offsetViewportHeight;
    static int offsetPanels;
};
