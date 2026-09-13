#include "CExoSound.h"

CExoSound::EnableNonStreamingEnvironmentEffectsFn CExoSound::enableNonStreamingEnvironmentEffects = nullptr;
CExoSound::EnableStreamingEnvironmentEffectsFn CExoSound::enableStreamingEnvironmentEffects = nullptr;
CExoSound::SetDesiredEnvironmentFn CExoSound::setDesiredEnvironment = nullptr;

CExoSound::GetBestEAXAvailableFn CExoSound::getBestEAXAvailable = nullptr;
CExoSound::GetEAXFn CExoSound::getEAX = nullptr;
CExoSound::GetSoundModeFn CExoSound::getSoundMode = nullptr;
CExoSound::GetLostFocusPausedFn CExoSound::getLostFocusPaused = nullptr;
CExoSound::ReInitializeFn CExoSound::reInitialize = nullptr;

CExoSound::Get2D3DBiasFn CExoSound::get2D3DBias = nullptr;
CExoSound::Set2D3DBiasFn CExoSound::set2D3DBias = nullptr;

CExoSound::GetListenerPositionFn CExoSound::getListenerPosition = nullptr;
CExoSound::SetListenerOrientationFn CExoSound::setListenerOrientation = nullptr;
CExoSound::SetListenerPositionFn CExoSound::setListenerPosition = nullptr;

CExoSound::GetNumber2DVoicesFn CExoSound::getNumber2DVoices = nullptr;
CExoSound::GetNumber3DVoicesFn CExoSound::getNumber3DVoices = nullptr;
CExoSound::GetPriorityGroupDistanceFn CExoSound::getPriorityGroupDistance = nullptr;

CExoSound::SetDesiredAmbientScaleFn CExoSound::setDesiredAmbientScale = nullptr;
CExoSound::SetDialogVolumeFn CExoSound::setDialogVolume = nullptr;
CExoSound::SetSoundEffectVolumeFn CExoSound::setSoundEffectVolume = nullptr;

CExoSound::StopAllOneShotsFn CExoSound::stopAllOneShots = nullptr;

bool CExoSound::functionsInitialized = false;
bool CExoSound::offsetsInitialized = false;

void CExoSound::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoSound] ERROR: GameVersion not initialized\n");
        return;
    }

    GameVersion::ResolveFunction(enableNonStreamingEnvironmentEffects, "CExoSound", "EnableNonStreamingEnvironmentEffects");
    GameVersion::ResolveFunction(enableStreamingEnvironmentEffects, "CExoSound", "EnableStreamingEnvironmentEffects");
    GameVersion::ResolveFunction(setDesiredEnvironment, "CExoSound", "SetDesiredEnvironment");

    GameVersion::ResolveFunction(getBestEAXAvailable, "CExoSound", "GetBestEAXAvailable");
    GameVersion::ResolveFunction(getEAX, "CExoSound", "GetEAX");
    GameVersion::ResolveFunction(getSoundMode, "CExoSound", "GetSoundMode");
    GameVersion::ResolveFunction(getLostFocusPaused, "CExoSound", "GetLostFocusPaused");
    GameVersion::ResolveFunction(reInitialize, "CExoSound", "ReInitialize");

    GameVersion::ResolveFunction(get2D3DBias, "CExoSound", "Get2D3DBias");
    GameVersion::ResolveFunction(set2D3DBias, "CExoSound", "Set2D3DBias");

    GameVersion::ResolveFunction(getListenerPosition, "CExoSound", "GetListenerPosition");
    GameVersion::ResolveFunction(setListenerOrientation, "CExoSound", "SetListenerOrientation");
    GameVersion::ResolveFunction(setListenerPosition, "CExoSound", "SetListenerPosition");

    GameVersion::ResolveFunction(getNumber2DVoices, "CExoSound", "GetNumber2DVoices");
    GameVersion::ResolveFunction(getNumber3DVoices, "CExoSound", "GetNumber3DVoices");
    GameVersion::ResolveFunction(getPriorityGroupDistance, "CExoSound", "GetPriorityGroupDistance");

    GameVersion::ResolveFunction(setDesiredAmbientScale, "CExoSound", "SetDesiredAmbientScale");
    GameVersion::ResolveFunction(setDialogVolume, "CExoSound", "SetDialogVolume");
    GameVersion::ResolveFunction(setSoundEffectVolume, "CExoSound", "SetSoundEffectVolume");

    GameVersion::ResolveFunction(stopAllOneShots, "CExoSound", "StopAllOneShots");

    functionsInitialized = true;
}

void CExoSound::InitializeOffsets() {
    // TODO: CExoSound offsets not wrapped yet
    offsetsInitialized = true;
}

CExoSound::CExoSound()
    : GameAPIObject(nullptr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoSound] ERROR: GameVersion not initialized\n");
        return;
    }

    // ExoSound holds the ADDRESS of the global that stores the sound manager
    // pointer, so dereference once to reach the actual instance.
    void** soundGlobalPtr = static_cast<void**>(GameVersion::GetGlobalPointer("ExoSound"));
    if (soundGlobalPtr && *soundGlobalPtr) {
        objectPtr = *soundGlobalPtr;
    } else {
        OutputDebugStringA("[CExoSound] ERROR: ExoSound is null\n");
    }
}

CExoSound::CExoSound(void* soundPtr)
    : GameAPIObject(soundPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CExoSound::~CExoSound()
{
    // Base class destructor handles objectPtr cleanup
}

void CExoSound::EnableNonStreamingEnvironmentEffects(int enabled) {
    if (!objectPtr || !enableNonStreamingEnvironmentEffects) return;
    enableNonStreamingEnvironmentEffects(objectPtr, enabled);
}

void CExoSound::EnableStreamingEnvironmentEffects(int enabled) {
    if (!objectPtr || !enableStreamingEnvironmentEffects) return;
    enableStreamingEnvironmentEffects(objectPtr, enabled);
}

void CExoSound::SetDesiredEnvironment(int env) {
    if (!objectPtr || !setDesiredEnvironment) return;
    setDesiredEnvironment(objectPtr, env);
}

int CExoSound::GetBestEAXAvailable() {
    if (!objectPtr || !getBestEAXAvailable) return 0;
    return getBestEAXAvailable(objectPtr);
}

int CExoSound::GetEAX() {
    if (!objectPtr || !getEAX) return 0;
    return getEAX(objectPtr);
}

int CExoSound::GetSoundMode() {
    if (!objectPtr || !getSoundMode) return 0;
    return getSoundMode(objectPtr);
}

int CExoSound::GetLostFocusPaused() {
    if (!objectPtr || !getLostFocusPaused) return 0;
    return getLostFocusPaused(objectPtr);
}

void CExoSound::ReInitialize(int eax, int allowHardwareAudio) {
    if (!objectPtr || !reInitialize) return;
    reInitialize(objectPtr, eax, allowHardwareAudio);
}

// x87 FPU call: return comes back in ST(0), use CallFPUFunction.
float10 CExoSound::Get2D3DBias() {
    if (!objectPtr || !get2D3DBias) return 0.0f;
    // Use FPU wrapper for x87 calling convention (returns in ST(0))
    return CallFPUFunction(get2D3DBias, objectPtr);
}

void CExoSound::Set2D3DBias(float bias) {
    if (!objectPtr || !set2D3DBias) return;
    set2D3DBias(objectPtr, bias);
}

void CExoSound::GetListenerPosition(Vector* outPosition) {
    if (!objectPtr || !getListenerPosition) return;
    getListenerPosition(objectPtr, outPosition);
}

void CExoSound::SetListenerOrientation(Vector* orientForward, Vector* orientUp) {
    if (!objectPtr || !setListenerOrientation) return;
    setListenerOrientation(objectPtr, orientForward, orientUp);
}

void CExoSound::SetListenerPosition(Vector* position) {
    if (!objectPtr || !setListenerPosition) return;
    setListenerPosition(objectPtr, position);
}

BYTE CExoSound::GetNumber2DVoices() {
    if (!objectPtr || !getNumber2DVoices) return 0;
    return getNumber2DVoices(objectPtr);
}

BYTE CExoSound::GetNumber3DVoices() {
    if (!objectPtr || !getNumber3DVoices) return 0;
    return getNumber3DVoices(objectPtr);
}

void CExoSound::GetPriorityGroupDistance(BYTE groupIndex, float* maxVolDist, float* minVolDist) {
    if (!objectPtr || !getPriorityGroupDistance) return;
    getPriorityGroupDistance(objectPtr, groupIndex, maxVolDist, minVolDist);
}

void CExoSound::SetDesiredAmbientScale(float scale) {
    if (!objectPtr || !setDesiredAmbientScale) return;
    setDesiredAmbientScale(objectPtr, scale);
}

void CExoSound::SetDialogVolume(float volume) {
    if (!objectPtr || !setDialogVolume) return;
    setDialogVolume(objectPtr, volume);
}

void CExoSound::SetSoundEffectVolume(float volume) {
    if (!objectPtr || !setSoundEffectVolume) return;
    setSoundEffectVolume(objectPtr, volume);
}

void CExoSound::StopAllOneShots() {
    if (!objectPtr || !stopAllOneShots) return;
    stopAllOneShots(objectPtr);
}
