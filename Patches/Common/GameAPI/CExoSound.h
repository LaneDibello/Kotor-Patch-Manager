#pragma once
#include "GameVersion.h"
#include "GameAPIObject.h"
#include "../Common.h"

/// <summary>
/// GameAPI wrapper for CExoSound - the engine's sound manager.
/// Owns EAX/environment settings, listener placement, voice counts and volumes.
/// </summary>
class CExoSound : public GameAPIObject {
public:
    // Automatically retrieves and wraps the global sound manager (ExoSound).
    CExoSound();
    explicit CExoSound(void* soundPtr);
    ~CExoSound();

    // Environment effects
    void EnableNonStreamingEnvironmentEffects(int enabled);
    void EnableStreamingEnvironmentEffects(int enabled);
    void SetDesiredEnvironment(int env);

    // EAX / mode
    int GetBestEAXAvailable();
    int GetEAX();
    int GetSoundMode();
    int GetLostFocusPaused();
    void ReInitialize(int eax, int allowHardwareAudio);

    // 2D/3D bias
    // x87 FPU call: return comes back in ST(0), use CallFPUFunction.
    float10 Get2D3DBias();
    void Set2D3DBias(float bias);

    // Listener
    void GetListenerPosition(Vector* outPosition);
    void SetListenerOrientation(Vector* orientForward, Vector* orientUp);
    void SetListenerPosition(Vector* position);

    // Voices / priority groups
    BYTE GetNumber2DVoices();
    BYTE GetNumber3DVoices();
    void GetPriorityGroupDistance(BYTE groupIndex, float* maxVolDist, float* minVolDist);

    // Volumes
    void SetDesiredAmbientScale(float scale);
    void SetDialogVolume(float volume);
    void SetSoundEffectVolume(float volume);

    // Playback
    void StopAllOneShots();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef void (__thiscall* EnableNonStreamingEnvironmentEffectsFn)(void* thisPtr, int enabled);
    typedef void (__thiscall* EnableStreamingEnvironmentEffectsFn)(void* thisPtr, int enabled);
    typedef void (__thiscall* SetDesiredEnvironmentFn)(void* thisPtr, int env);

    typedef int (__thiscall* GetBestEAXAvailableFn)(void* thisPtr);
    typedef int (__thiscall* GetEAXFn)(void* thisPtr);
    typedef int (__thiscall* GetSoundModeFn)(void* thisPtr);
    typedef int (__thiscall* GetLostFocusPausedFn)(void* thisPtr);
    typedef void (__thiscall* ReInitializeFn)(void* thisPtr, int eax, int allowHardwareAudio);

    typedef float10 (__thiscall* Get2D3DBiasFn)(void* thisPtr);
    typedef void (__thiscall* Set2D3DBiasFn)(void* thisPtr, float bias);

    typedef void (__thiscall* GetListenerPositionFn)(void* thisPtr, Vector* outPosition);
    typedef void (__thiscall* SetListenerOrientationFn)(void* thisPtr, Vector* orientForward, Vector* orientUp);
    typedef void (__thiscall* SetListenerPositionFn)(void* thisPtr, Vector* position);

    typedef BYTE (__thiscall* GetNumber2DVoicesFn)(void* thisPtr);
    typedef BYTE (__thiscall* GetNumber3DVoicesFn)(void* thisPtr);
    typedef void (__thiscall* GetPriorityGroupDistanceFn)(void* thisPtr, BYTE groupIndex, float* maxVolDist, float* minVolDist);

    typedef void (__thiscall* SetDesiredAmbientScaleFn)(void* thisPtr, float scale);
    typedef void (__thiscall* SetDialogVolumeFn)(void* thisPtr, float volume);
    typedef void (__thiscall* SetSoundEffectVolumeFn)(void* thisPtr, float volume);

    typedef void (__thiscall* StopAllOneShotsFn)(void* thisPtr);

    static EnableNonStreamingEnvironmentEffectsFn enableNonStreamingEnvironmentEffects;
    static EnableStreamingEnvironmentEffectsFn enableStreamingEnvironmentEffects;
    static SetDesiredEnvironmentFn setDesiredEnvironment;

    static GetBestEAXAvailableFn getBestEAXAvailable;
    static GetEAXFn getEAX;
    static GetSoundModeFn getSoundMode;
    static GetLostFocusPausedFn getLostFocusPaused;
    static ReInitializeFn reInitialize;

    static Get2D3DBiasFn get2D3DBias;
    static Set2D3DBiasFn set2D3DBias;

    static GetListenerPositionFn getListenerPosition;
    static SetListenerOrientationFn setListenerOrientation;
    static SetListenerPositionFn setListenerPosition;

    static GetNumber2DVoicesFn getNumber2DVoices;
    static GetNumber3DVoicesFn getNumber3DVoices;
    static GetPriorityGroupDistanceFn getPriorityGroupDistance;

    static SetDesiredAmbientScaleFn setDesiredAmbientScale;
    static SetDialogVolumeFn setDialogVolume;
    static SetSoundEffectVolumeFn setSoundEffectVolume;

    static StopAllOneShotsFn stopAllOneShots;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
