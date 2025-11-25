#pragma once

/// <summary>
/// Abstract base class for all GameAPI wrapper classes.
/// Provides consistent pointer management, initialization interface,
/// and memory handling for objects that wrap game engine memory.
/// </summary>
class GameAPIObject {
public:
    /// <summary>
    /// Constructs a GameAPIObject that wraps an existing game memory address.
    /// </summary>
    /// <param name="objectPtr">Pointer to game memory to wrap</param>
    /// <param name="shouldFree">If true, destructor will free the memory (default: false)</param>
    GameAPIObject(void* objectPtr, bool shouldFree = false);

    /// <summary>
    /// Virtual destructor. If shouldFree is true, derived classes should
    /// call appropriate game destructors before memory is freed.
    /// </summary>
    virtual ~GameAPIObject();

    /// <summary>
    /// Gets the pointer to the underlying game memory.
    /// </summary>
    /// <returns>Pointer to game object, or nullptr if invalid</returns>
    void* GetPtr() const;

    /// <summary>
    /// Checks if the wrapped pointer is valid (non-null).
    /// </summary>
    /// <returns>True if objectPtr is not nullptr</returns>
    bool IsValid() const;

    /// <summary>
    /// Pure virtual function that derived classes must implement to
    /// initialize static function pointers via GameVersion lookups.
    /// Called once per class on first instantiation.
    /// </summary>
    virtual void InitializeFunctions() = 0;

    /// <summary>
    /// Pure virtual function that derived classes must implement to
    /// initialize static offset values via GameVersion lookups.
    /// Called once per class on first instantiation.
    /// </summary>
    virtual void InitializeOffsets() = 0;

protected:
    /// <summary>
    /// Pointer to the game engine memory this object wraps.
    /// Protected so derived classes can access directly.
    /// </summary>
    void* objectPtr;

    /// <summary>
    /// Flag indicating whether this wrapper owns the memory and should
    /// free it in the destructor. True when we allocate game objects,
    /// false when wrapping existing game memory (the common case).
    /// </summary>
    bool shouldFree;

private:
    // Prevent copying (game objects should not be copied)
    GameAPIObject(const GameAPIObject&) = delete;
    GameAPIObject& operator=(const GameAPIObject&) = delete;
};
