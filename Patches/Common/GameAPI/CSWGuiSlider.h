#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiBorder;
class CSWGuiBorderParams;
class CSWGuiImage;
class CResRef;
struct CSWGuiExtent;

// CSWGuiSlider virtual-function table for KotOR 1 (Windows): 40 entries.
// Slots 0..37 keep their ControlVTableSlot indices (see CSWGuiControl.h); these
// are the slider's additions.
//
// Read out of swkotor.exe 1.03 at the vtable, 0x73E9D0: slot 38 holds Initialize_2
// (0x418EF0), slot 39 holds Initialize (0x417DC0), and slot 40 is already string
// data, so the table ends there. The gap to the next class's vtable is not the
// slot count -- the tables are not packed.
enum class SliderVTableSlot : int {
	InitializeFromProto = 38,
	Initialize = 39
};

inline constexpr int SLIDER_VTABLE_SLOT_COUNT = 40;

// The slider has no minimum: cur_value runs 0..max_value, and every code path in
// the game (HandleInputEvent, HandleMouseCapturedMovement, SetExtent) treats 0 as
// the floor. A caller wanting a non-zero minimum applies it as an offset itself.
class CSWGuiSlider : public CSWGuiNavigable {
public:
	explicit CSWGuiSlider(void* objectPtr);
	CSWGuiSlider();
	~CSWGuiSlider();

	// Accessors. Returned wrapper is heap allocated; caller owns it.
	CSWGuiBorder* GetBorder();
	CSWGuiBorder* GetBorderHilight();
	CSWGuiImage* GetImage();
	int GetMaxValue();
	int GetCurValue();
	// Index of the sound the slider plays when a step actually moves it.
	BYTE GetGuiSound();
	void SetGuiSound(BYTE sound);

	// Functions
	void Initialize(CSWGuiExtent* extent, CSWGuiBorderParams* borderParams,
	                CSWGuiBorderParams* borderHilightParams, CResRef* imageResRef);
	void SetExtent(CSWGuiExtent* extent);
	void SetMaxValue(int maxValue);
	void SetCurValue(int curValue);
	void Draw(float alpha);
	void HandleInputEvent(int event, int doPanelEvents);
	// 1 thumb, 2 track before it, 3 track after it, 0 miss. Coordinates are local
	// to the control's gui object (offset 0x34), not the screen.
	int HitCheckSlider(int x, int y);

	void InitializeFunctions() override;
	void InitializeOffsets() override;

	int VTableSlotCount() override;

protected:
	typedef void (__thiscall* InitializeFn)(void* thisPtr, void* extent, void* borderParams,
	                                        void* borderHilightParams, void* imageResRef);
	typedef void (__thiscall* SetExtentFn)(void* thisPtr, void* extent);
	typedef void (__thiscall* SetMaxValueFn)(void* thisPtr, int maxValue);
	typedef void (__thiscall* SetCurValueFn)(void* thisPtr, int curValue);
	typedef void (__thiscall* DrawFn)(void* thisPtr, float alpha);
	typedef void (__thiscall* HandleInputEventFn)(void* thisPtr, int event, int doPanelEvents);
	typedef int (__thiscall* HitCheckSliderFn)(void* thisPtr, int x, int y);
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);

	static InitializeFn initialize;
	static SetExtentFn setExtent;
	static SetMaxValueFn setMaxValue;
	static SetCurValueFn setCurValue;
	static DrawFn draw;
	static HandleInputEventFn handleInputEvent;
	static HitCheckSliderFn hitCheckSlider;
	static ConstructorFn constructor;
	static DestructorFn destructor;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetMaxValue;
	static int offsetCurValue;
	static int offsetGuiSound;
	static int offsetBorder;
	static int offsetBorderHilight;
	static int offsetImage;
};
