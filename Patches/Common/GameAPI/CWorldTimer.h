#pragma once
#include <windows.h>
#include "GameAPIObject.h"

class CWorldTimer : public GameAPIObject {
public:
    // Wraps an existing CWorldTimer pointer (does not take ownership).
    explicit CWorldTimer(void* objectPtr);
    ~CWorldTimer();

    // World time arithmetic
    int AddWorldTimes(DWORD lhsDays, DWORD lhsMS, DWORD rhsDays, DWORD rhsMS, DWORD* outDays, DWORD* outMS);
    int SubtractWorldTimes(DWORD lhsDays, DWORD lhsMS, DWORD rhsDays, DWORD rhsMS, DWORD* outDays, DWORD* outMS);
    int CompareWorldTimes(DWORD lhsDay, DWORD lhsMS, DWORD rhsDay, DWORD rhsMS);

    // Conversions
    void ConvertFromCalendarDay(DWORD calendarDay, DWORD* yearOut, DWORD* monthOut, DWORD* dayOut);
    void ConvertFromTimeOfDay(DWORD timeInMS, DWORD* outHours, DWORD* outMinutes, DWORD* outSeconds, DWORD* outMS);
    int ConvertHourMinSecMsToMs(DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds);
    int GetCalendarDayFromSeconds(float seconds);

    // Current world time
    void GetWorldTime(DWORD* outDays, DWORD* outMS);
    int GetWorldTimeHour();
    int GetWorldTimeMinute();
    int GetWorldTimeSecond();
    int GetWorldTimeMillisecond();
    void AdvanceToTime(int hours, int minutes, int seconds, int milliseconds);

    // Timer control
    void PauseWorldTimer();
    void UnpauseWorldTimer();
    void ResetTimer(CWorldTimer* reset);
    void SetMinutesPerHour(BYTE minutesPerHour);
    void SetSnapshotScale(float scale);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef int(__thiscall* AddWorldTimesFn)(void* thisPtr, DWORD lhsDays, DWORD lhsMS, DWORD rhsDays, DWORD rhsMS, DWORD* outDays, DWORD* outMS);
    typedef int(__thiscall* SubtractWorldTimesFn)(void* thisPtr, DWORD lhsDays, DWORD lhsMS, DWORD rhsDays, DWORD rhsMS, DWORD* outDays, DWORD* outMS);
    typedef int(__thiscall* CompareWorldTimesFn)(void* thisPtr, DWORD lhsDay, DWORD lhsMS, DWORD rhsDay, DWORD rhsMS);

    typedef void(__thiscall* ConvertFromCalendarDayFn)(void* thisPtr, DWORD calendarDay, DWORD* yearOut, DWORD* monthOut, DWORD* dayOut);
    typedef void(__thiscall* ConvertFromTimeOfDayFn)(void* thisPtr, DWORD timeInMS, DWORD* outHours, DWORD* outMinutes, DWORD* outSeconds, DWORD* outMS);
    typedef int(__thiscall* ConvertHourMinSecMsToMsFn)(void* thisPtr, DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds);
    typedef int(__thiscall* GetCalendarDayFromSecondsFn)(void* thisPtr, float seconds);

    typedef void(__thiscall* GetWorldTimeFn)(void* thisPtr, DWORD* outDays, DWORD* outMS);
    typedef int(__thiscall* GetWorldTimeHourFn)(void* thisPtr);
    typedef int(__thiscall* GetWorldTimeMinuteFn)(void* thisPtr);
    typedef int(__thiscall* GetWorldTimeSecondFn)(void* thisPtr);
    typedef int(__thiscall* GetWorldTimeMillisecondFn)(void* thisPtr);
    typedef void(__thiscall* AdvanceToTimeFn)(void* thisPtr, int hours, int minutes, int seconds, int milliseconds);

    typedef void(__thiscall* PauseWorldTimerFn)(void* thisPtr);
    typedef void(__thiscall* UnpauseWorldTimerFn)(void* thisPtr);
    typedef void(__thiscall* ResetTimerFn)(void* thisPtr, void* reset);
    typedef void(__thiscall* SetMinutesPerHourFn)(void* thisPtr, BYTE minutesPerHour);
    typedef void(__thiscall* SetSnapshotScaleFn)(void* thisPtr, float scale);

    static AddWorldTimesFn addWorldTimes;
    static SubtractWorldTimesFn subtractWorldTimes;
    static CompareWorldTimesFn compareWorldTimes;

    static ConvertFromCalendarDayFn convertFromCalendarDay;
    static ConvertFromTimeOfDayFn convertFromTimeOfDay;
    static ConvertHourMinSecMsToMsFn convertHourMinSecMsToMs;
    static GetCalendarDayFromSecondsFn getCalendarDayFromSeconds;

    static GetWorldTimeFn getWorldTime;
    static GetWorldTimeHourFn getWorldTimeHour;
    static GetWorldTimeMinuteFn getWorldTimeMinute;
    static GetWorldTimeSecondFn getWorldTimeSecond;
    static GetWorldTimeMillisecondFn getWorldTimeMillisecond;
    static AdvanceToTimeFn advanceToTime;

    static PauseWorldTimerFn pauseWorldTimer;
    static UnpauseWorldTimerFn unpauseWorldTimer;
    static ResetTimerFn resetTimer;
    static SetMinutesPerHourFn setMinutesPerHour;
    static SetSnapshotScaleFn setSnapshotScale;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
