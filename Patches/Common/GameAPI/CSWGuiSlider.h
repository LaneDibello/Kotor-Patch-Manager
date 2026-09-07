#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiBorder;
class CSWGuiBorderParams;
class CSWGuiImage;
class CResRef;
struct CSWGuiExtent;

// CSWGuiSlider vtable, KotOR 1 (Windows): 40 entries. Slots 0..37 keep their
// ControlVTableSlot indices; these are the slider's additions. Read out of the
// vtable at 0x73E9D0 -- slot 40 is string data, so the table ends there.
enum class SliderVTableSlot : int {
	InitializeFromProto = 38,
	Initialize = 39
};

inline constexpr int SLIDER_VTABLE_SLOT_COUNT = 40;

// No minimum: cur_value runs 0..max_value, and every code path treats 0 as the
// floor. A caller wanting a non-zero minimum applies it as an offset itself.
class CSWGuiSlider : public CSWGuiNavigable {
public:
	// Track clicks either side of the thumb. HandleLMouseDown synthesises these and
	// feeds them back into HandleInputEvent; they are private to the slider, as
	// other controls reuse the same ids for their own purposes.
	static const int EVENT_TRACK_UP = 500;
	static const int EVENT_TRACK_DOWN = 501;

	explicit CSWGuiSlider(void* objectPtr);
	CSWGuiSlider();
	~CSWGuiSlider();

	// Accessors. Returned wrapper is heap allocated; caller owns it.
	CSWGuiBorder* GetBorder();
	CSWGuiBorder* GetBorderHilight();
	CSWGuiImage* GetImage();
	int GetMaxValue();
	int GetCurValue();
	// Sound played when a step actually moves the thumb.
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
	// to the control's gui object, not the screen.
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
