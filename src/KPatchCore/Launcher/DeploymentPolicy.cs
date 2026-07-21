using System.Runtime.InteropServices;
using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// How KotorPatcher gets loaded into the game.
/// </summary>
public enum DeploymentMethod
{
    /// <summary>Runtime DLL injection into the launched process.</summary>
    Injection,

    /// <summary>
    /// The KProxy loads the patcher when the game starts. Works under
    /// Wine/Proton, and on Windows too.
    /// </summary>
    Proxy,

    /// <summary>
    /// KotorPatcher.so is added to the native Linux game ELF's DT_NEEDED list, so
    /// the dynamic loader maps it at startup. No injector or proxy needed.
    /// </summary>
    ElfNeeded,
}

/// <summary>
/// The single decision point for the deployment method. The proxy machinery
/// (staging, launch) is platform-agnostic; this is the one place its use is wired
/// up, so it can stay unused on Windows without being coupled to Linux everywhere.
/// </summary>
public static class DeploymentPolicy
{
    /// <summary>
    /// The deployment method for the current platform. Linux must use the proxy
    /// (native injection can't reach a Wine process); Windows uses injection.
    /// </summary>
    /// <remarks>
    /// To have Windows use the KProxy too, return
    /// <see cref="DeploymentMethod.Proxy"/> here. Nothing else is platform-gated.
    /// </remarks>
    public static DeploymentMethod ForCurrentPlatform()
    {
        return RuntimeInformation.IsOSPlatform(OSPlatform.Linux)
            ? DeploymentMethod.Proxy
            : DeploymentMethod.Injection;
    }

    /// <summary>
    /// The deployment method for a specific detected game. A native Linux ELF uses
    /// <see cref="DeploymentMethod.ElfNeeded"/> (the loader maps the patcher via
    /// DT_NEEDED); everything else is a Windows PE and falls back to the host default
    /// (proxy under Wine/Proton, injection on Windows).
    /// </summary>
    public static DeploymentMethod ForGame(GameVersion gameVersion)
    {
        return gameVersion.Platform == Platform.Linux
            ? DeploymentMethod.ElfNeeded
            : ForCurrentPlatform();
    }
}
