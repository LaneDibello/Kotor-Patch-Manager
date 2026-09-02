#include "CWorldTimer.h"
#include "GameVersion.h"
#include "../Common.h"

CWorldTimer::AddWorldTimesFn CWorldTimer::addWorldTimes = nullptr;
CWorldTimer::SubtractWorldTimesFn CWorldTimer::subtractWorldTimes = nullptr;
CWorldTimer::CompareWorldTimesFn CWorldTimer::compareWorldTimes = nullptr;

CWorldTimer::ConvertFromCalendarDayFn CWorldTimer::convertFromCalendarDay = nullptr;
CWorldTimer::ConvertFromTimeOfDayFn CWorldTimer::convertFromTimeOfDay = nullptr;
CWorldTimer::ConvertHourMinSecMsToMsFn CWorldTimer::convertHourMinSecMsToMs = nullptr;
CWorldTimer::GetCalendarDayFromSecondsFn CWorldTimer::getCalendarDayFromSeconds = nullptr;

CWorldTimer::GetWorldTimeFn CWorldTimer::getWorldTime = nullptr;
CWorldTimer::GetWorldTimeHourFn CWorldTimer::getWorldTimeHour = nullptr;
CWorldTimer::GetWorldTimeMinuteFn CWorldTimer::getWorldTimeMinute = nullptr;
CWorldTimer::GetWorldTimeSecondFn CWorldTimer::getWorldTimeSecond = nullptr;
CWorldTimer::GetWorldTimeMillisecondFn CWorldTimer::getWorldTimeMillisecond = nullptr;
CWorldTimer::AdvanceToTimeFn CWorldTimer::advanceToTime = nullptr;

CWorldTimer::PauseWorldTimerFn CWorldTimer::pauseWorldTimer = nullptr;
CWorldTimer::UnpauseWorldTimerFn CWorldTimer::unpauseWorldTimer = nullptr;
CWorldTimer::ResetTimerFn CWorldTimer::resetTimer = nullptr;
CWorldTimer::SetMinutesPerHourFn CWorldTimer::setMinutesPerHour = nullptr;
CWorldTimer::SetSnapshotScaleFn CWorldTimer::setSnapshotScale = nullptr;

bool CWorldTimer::functionsInitialized = false;
bool CWorldTimer::offsetsInitialized = false;

void CWorldTimer::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CWorldTimer] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addWorldTimes = reinterpret_cast<AddWorldTimesFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "AddWorldTimes"));
        subtractWorldTimes = reinterpret_cast<SubtractWorldTimesFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "SubtractWorldTimes"));
        compareWorldTimes = reinterpret_cast<CompareWorldTimesFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "CompareWorldTimes"));

        convertFromCalendarDay = reinterpret_cast<ConvertFromCalendarDayFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "ConvertFromCalendarDay"));
        convertFromTimeOfDay = reinterpret_cast<ConvertFromTimeOfDayFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "ConvertFromTimeOfDay"));
        convertHourMinSecMsToMs = reinterpret_cast<ConvertHourMinSecMsToMsFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "ConvertHourMinSecMsToMs"));
        getCalendarDayFromSeconds = reinterpret_cast<GetCalendarDayFromSecondsFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "GetCalendarDayFromSeconds"));

        getWorldTime = reinterpret_cast<GetWorldTimeFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "GetWorldTime"));
        getWorldTimeHour = reinterpret_cast<GetWorldTimeHourFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "GetWorldTimeHour"));
        getWorldTimeMinute = reinterpret_cast<GetWorldTimeMinuteFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "GetWorldTimeMinute"));
        getWorldTimeSecond = reinterpret_cast<GetWorldTimeSecondFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "GetWorldTimeSecond"));
        getWorldTimeMillisecond = reinterpret_cast<GetWorldTimeMillisecondFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "GetWorldTimeMillisecond"));
        advanceToTime = reinterpret_cast<AdvanceToTimeFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "AdvanceToTime"));

        pauseWorldTimer = reinterpret_cast<PauseWorldTimerFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "PauseWorldTimer"));
        unpauseWorldTimer = reinterpret_cast<UnpauseWorldTimerFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "UnpauseWorldTimer"));
        resetTimer = reinterpret_cast<ResetTimerFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "ResetTimer"));
        setMinutesPerHour = reinterpret_cast<SetMinutesPerHourFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "SetMinutesPerHour"));
        setSnapshotScale = reinterpret_cast<SetSnapshotScaleFn>(
            GameVersion::GetFunctionAddress("CWorldTimer", "SetSnapshotScale"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CWorldTimer] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CWorldTimer::InitializeOffsets() {
    // CWorldTimer exposes no offsets through this wrapper (functions only)
    offsetsInitialized = true;
}

CWorldTimer::CWorldTimer(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (owned by the game)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CWorldTimer::~CWorldTimer() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== World time arithmetic =====

int CWorldTimer::AddWorldTimes(DWORD lhsDays, DWORD lhsMS, DWORD rhsDays, DWORD rhsMS, DWORD* outDays, DWORD* outMS) {
    if (!objectPtr || !addWorldTimes) {
        return 0;
    }
    return addWorldTimes(objectPtr, lhsDays, lhsMS, rhsDays, rhsMS, outDays, outMS);
}

int CWorldTimer::SubtractWorldTimes(DWORD lhsDays, DWORD lhsMS, DWORD rhsDays, DWORD rhsMS, DWORD* outDays, DWORD* outMS) {
    if (!objectPtr || !subtractWorldTimes) {
        return 0;
    }
    return subtractWorldTimes(objectPtr, lhsDays, lhsMS, rhsDays, rhsMS, outDays, outMS);
}

int CWorldTimer::CompareWorldTimes(DWORD lhsDay, DWORD lhsMS, DWORD rhsDay, DWORD rhsMS) {
    if (!objectPtr || !compareWorldTimes) {
        return 0;
    }
    return compareWorldTimes(objectPtr, lhsDay, lhsMS, rhsDay, rhsMS);
}

// ===== Conversions =====

void CWorldTimer::ConvertFromCalendarDay(DWORD calendarDay, DWORD* yearOut, DWORD* monthOut, DWORD* dayOut) {
    if (!objectPtr || !convertFromCalendarDay) {
        return;
    }
    convertFromCalendarDay(objectPtr, calendarDay, yearOut, monthOut, dayOut);
}

void CWorldTimer::ConvertFromTimeOfDay(DWORD timeInMS, DWORD* outHours, DWORD* outMinutes, DWORD* outSeconds, DWORD* outMS) {
    if (!objectPtr || !convertFromTimeOfDay) {
        return;
    }
    convertFromTimeOfDay(objectPtr, timeInMS, outHours, outMinutes, outSeconds, outMS);
}

int CWorldTimer::ConvertHourMinSecMsToMs(DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds) {
    if (!objectPtr || !convertHourMinSecMsToMs) {
        return 0;
    }
    return convertHourMinSecMsToMs(objectPtr, hours, minutes, seconds, milliseconds);
}

int CWorldTimer::GetCalendarDayFromSeconds(float seconds) {
    if (!objectPtr || !getCalendarDayFromSeconds) {
        return 0;
    }
    return getCalendarDayFromSeconds(objectPtr, seconds);
}

// ===== Current world time =====

void CWorldTimer::GetWorldTime(DWORD* outDays, DWORD* outMS) {
    if (!objectPtr || !getWorldTime) {
        return;
    }
    getWorldTime(objectPtr, outDays, outMS);
}

int CWorldTimer::GetWorldTimeHour() {
    if (!objectPtr || !getWorldTimeHour) {
        return 0;
    }
    return getWorldTimeHour(objectPtr);
}

int CWorldTimer::GetWorldTimeMinute() {
    if (!objectPtr || !getWorldTimeMinute) {
        return 0;
    }
    return getWorldTimeMinute(objectPtr);
}

int CWorldTimer::GetWorldTimeSecond() {
    if (!objectPtr || !getWorldTimeSecond) {
        return 0;
    }
    return getWorldTimeSecond(objectPtr);
}

int CWorldTimer::GetWorldTimeMillisecond() {
    if (!objectPtr || !getWorldTimeMillisecond) {
        return 0;
    }
    return getWorldTimeMillisecond(objectPtr);
}

void CWorldTimer::AdvanceToTime(int hours, int minutes, int seconds, int milliseconds) {
    if (!objectPtr || !advanceToTime) {
        return;
    }
    advanceToTime(objectPtr, hours, minutes, seconds, milliseconds);
}

// ===== Timer control =====

void CWorldTimer::PauseWorldTimer() {
    if (!objectPtr || !pauseWorldTimer) {
        return;
    }
    pauseWorldTimer(objectPtr);
}

void CWorldTimer::UnpauseWorldTimer() {
    if (!objectPtr || !unpauseWorldTimer) {
        return;
    }
    unpauseWorldTimer(objectPtr);
}

void CWorldTimer::ResetTimer(CWorldTimer* reset) {
    if (!objectPtr || !resetTimer) {
        return;
    }
    resetTimer(objectPtr, reset ? reset->GetPtr() : nullptr);
}

void CWorldTimer::SetMinutesPerHour(BYTE minutesPerHour) {
    if (!objectPtr || !setMinutesPerHour) {
        return;
    }
    setMinutesPerHour(objectPtr, minutesPerHour);
}

void CWorldTimer::SetSnapshotScale(float scale) {
    if (!objectPtr || !setSnapshotScale) {
        return;
    }
    setSnapshotScale(objectPtr, scale);
}
