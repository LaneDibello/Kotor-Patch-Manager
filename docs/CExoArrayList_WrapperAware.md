# Wrapper-Aware `CExoArrayList`

**Status**: Phase 1 implemented (rows 1–3); row 4 reserved
**Last Updated**: 2026-06-29

## Overview

`CExoArrayList<T>` (`Patches/Common/GameAPI/CExoArrayList.h`) wraps the game's
`CExoArrayList<T>` memory layout — `{ T* data (0x0), int size (0x4), int capacity (0x8) }`
— and reimplements the list logic itself (it does **not** call the game's
per-instantiation list functions, which are template-specialized and frequently
inlined away in the binary, so their addresses can't be reliably resolved).

This doc describes how the container marshals between our **GameAPI wrapper
objects** and the **raw game objects** the game's lists actually hold, so that
patch code can be written in the same spelling as the decompiled game code.

## The problem

A GameAPI wrapper (e.g. `CSWGuiButton`) is a *handle*: its layout is
`{ vtable*, void* objectPtr, bool shouldFree }`. It does **not** contain the game
object — it points at it. So:

- `CExoArrayList<CSWGuiButton>` (naively) would store whole handle structs inline,
  whose first bytes are a vtable pointer, not the game object. (It also can't
  compile — `GameAPIObject`'s copy constructor is `= delete`d.)
- `CExoArrayList<CSWGuiButton*>` (naively) would store pointers to our *handles*,
  not to the game controls.

But the game's `CExoArrayList<CSWGuiControl*>` holds an array of **raw game-object
pointers** — exactly what `wrapper.GetPtr()` returns. The container must bridge
that gap transparently.

## Design goal: mirror the game's spelling

The framework should read like the base game. In the decompiled code a control
list is always `CExoArrayList<CSWGuiControl*>`, and the game also uses by-value
lists for some types (e.g. `CExoArrayList<CSWRoomSurfaceMeshAABBNode>` alongside
`CExoArrayList<CSWRoomSurfaceMeshAABBNode*>`). We preserve both spellings rather
than collapsing them, so a patch author writes the same type they'd see in the
game source.

## The four cases

Dispatch is on two orthogonal axes: `std::is_pointer<T>` and
`std::is_base_of<GameAPIObject, std::remove_pointer_t<T>>`.

| # | Spelling | Backing buffer | `Add` stores | Read |
|---|----------|----------------|--------------|------|
| 1 | `CExoArrayList<int>`, `<CSWGuiExtent>` (POD value) | inline, `sizeof(T)` | the value | `operator[]` → `T&` |
| 2 | `CExoArrayList<void*>` (POD pointer) | inline, 4 bytes | the pointer value | `operator[]` → `T&` |
| 3 | `CExoArrayList<CSWGuiControl*>` (**wrapper pointer**) | raw game pointers, 4 bytes | `value->GetPtr()` | `ForEach` → `CSWGuiControl&` |
| 4 | `CExoArrayList<CSWGuiControl>` (**wrapper value**) | inline game objects, *game class size* | `memcpy(slot, value.GetPtr(), gameSize)` | wrapper over in-buffer object |

Rows 1–2 are the original POD behavior. Row 3 is the wrapper-pointer marshalling
(the common GUI case). Row 4 is reserved — see below.

## Phase 1 (implemented): rows 1–3

- **POD paths (1–2)** are unchanged. `sizeof(T)` already equals 4 for pointer
  `T`, so allocation/growth math is identical; only the *value* stored differs.
- **Wrapper-pointer path (3)**: a private `toStored(value)` helper returns
  `value->GetPtr()` (as the raw game pointer, reinterpreted to `T`) for
  wrapper-pointer `T`, or the value unchanged otherwise. Every element-ingesting
  method (`Add`, `AddUnique`, `Insert`, `Count`, `IndexOf`, `Remove`,
  `RemoveAll`) routes through it, so stores and comparisons all operate on raw
  game pointers.
- **Reads**: `ForEach(fn)` constructs a *stack*, non-owning wrapper per element
  and passes it by reference (`fn(CSWGuiControl& c)`). This avoids the
  non-copyable/non-movable handle problem (returning a wrapper by value won't
  compile, since `GameAPIObject` deletes copy and declares no move).
  `GetRaw(i)` returns the stored `void*` for explicit raw access.

Result — patch code matches the decompiled spelling:

```cpp
CExoArrayList<CSWGuiControl*> listButtons;
listButtons.Add(&redButton);          // &redButton: CSWGuiButton* -> CSWGuiControl*
listButtons.Add(&orangeButton);
testListBox.AddControls(&listButtons, 0, 1, 0);

// reading a control list returned by the game:
panel.GetControls()->ForEach([](CSWGuiControl& c) { c.SetActive(1); });
```

### `operator[]` caveat for wrapper lists

`operator[]` still returns `T&` (the raw slot) for all cases, so for a
wrapper-pointer list it yields the **raw game pointer** reinterpreted as
`CSWGuiControl*`. Treating that as a usable wrapper handle is unsafe — use
`ForEach`/`GetRaw` for wrapper lists. `operator[]` remains the natural accessor
for POD lists.

## Phase 2 (reserved): row 4, wrapper value storage

`CExoArrayList<CSWGuiControl>` (by-value storage of a `GameAPIObject`) is
currently blocked by a `static_assert`. Implementing it requires extra plumbing:

1. **Generic game class size.** Slots must be sized by the *game* object's size
   (`GameVersion::GetClassSize("CSWGuiControl")`), which each wrapper caches in
   its protected `classSize`. The template needs a uniform hook to read it — e.g.
   a public `static int GameClassSize()` per wrapper, or a `CExoElementTraits<T>`
   specialization — plus a way to trigger lazy initialization of that static
   without allocating a throwaway game object.
2. **Shallow-copy semantics.** Because we reimplement the list, `Add` would
   `memcpy` the game object's bytes (a shallow copy that bypasses the game type's
   real copy constructor). This is byte-identical to the game for
   trivially-copyable value types (most by-value game structs — geometry/AABB
   nodes — are plain floats/ints/indices), but diverges for types with
   non-trivial copy semantics, which we fundamentally cannot match without that
   type's copy-ctor address. The value path must document this.

Reserving the spelling now (with a clear `static_assert` message) means row 4 can
be added later with no API breakage.

## Realism boundaries

- We match the game's **spelling and memory layout**, not its per-type element
  ctor/copy/dtor semantics — those are ours (POD-accurate). Identical for the
  trivially-copyable elements that dominate real usage; divergent only for
  by-value elements with non-trivial copy/destroy.
- Reading wrappers back is via `ForEach` (no copy) rather than
  random-access-by-value, until/unless `GameAPIObject` is given proper handle
  move/copy semantics.
