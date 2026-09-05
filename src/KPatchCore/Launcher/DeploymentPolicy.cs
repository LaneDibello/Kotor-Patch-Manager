using System.Runtime.InteropServices;
using KPatchCore.Models;

namespace KPatchCore.Launcher;

/// <summary>
/// How KotorPatcher gets loaded into the game.
/// </summary>
/// <remarks>
/// Two of these have the game's own dynamic loader do the work, and they differ only in the
/// mechanism the executable format offers. <see cref="LinkedDependency"/> edits the executable's
/// dependency list, which is what ELF and Mach-O allow cheaply. <see cref="LibraryProxy"/> leaves
/// the executable alone and replaces a library it already loads, which is the answer wherever
/// editing that list is not an option. A packed executable is the case that forces it: the
/// SteamStub build of KOTOR 1 does not run the image sitting on disk, so nothing written there
/// would take effect. <see cref="RuntimeInjection"/> is the odd one out, changing nothing about
/// the game and requiring the launcher to be what starts it.
/// </remarks>
public enum DeploymentMethod
{
    /// <summary>
    /// The launcher starts the game and writes the patcher into the live process. Win32 only,
    /// and only available when the launcher is what started the game.
    /// </summary>
    RuntimeInjection,

    /// <summary>
    /// A library the game already loads is replaced with one of ours, which loads the patcher and
    /// forwards every call on to the original. The executable is untouched, so this survives
    /// packing and works under Wine/Proton. KProxy is the implementation, standing in for
    /// binkw32.dll.
    /// </summary>
    LibraryProxy,

    /// <summary>
    /// The patcher is named in the executable's own list of libraries to load, so the dynamic
    /// loader maps it at startup with no injector or proxy involved. That list is DT_NEEDED on
    /// ELF and LC_LOAD_DYLIB on Mach-O.
    /// </summary>
    LinkedDependency,
}

/// <summary>
/// The single decision point for the deployment method. The proxy machinery
/// (staging, launch) is platform-agnostic; this is the one place its use is wired
/// up, so it can stay unused on Windows without being coupled to Linux everywhere.
/// </summary>
public static class DeploymentPolicy
{
    /// <summary>
    /// Whether to reach the game through <see cref="DeploymentMethod.LibraryProxy"/> where
    /// injection would otherwise be used. Process-wide configuration, set from the user's saved
    /// settings at startup and whenever they change it. Linux is on the proxy either way, so this
    /// only decides anything on Windows.
    /// </summary>
    /// <remarks>
    /// Switching this while patches are installed would leave the game staged for the method it
    /// was installed with, so callers gate the change on there being nothing installed.
    /// </remarks>
    public static bool PreferLibraryProxy { get; set; }

    /// <summary>
    /// The deployment method for the current platform. Linux must use the proxy, since native
    /// injection cannot reach a Wine process; Windows injects unless asked for the proxy.
    /// </summary>
    /// <remarks>
    /// Windows follows <see cref="PreferLibraryProxy"/>. Nothing else is platform-gated.
    /// </remarks>
    public static DeploymentMethod ForCurrentPlatform()
    {
        // Injection is Win32 API work, so only a Windows host can do it at all. Everywhere else
        // the proxy is the only way into a Windows build of the game, whatever the user prefers.
        return RuntimeInformation.IsOSPlatform(OSPlatform.Windows) && !PreferLibraryProxy
            ? DeploymentMethod.RuntimeInjection
            : DeploymentMethod.LibraryProxy;
    }

    /// <summary>
    /// Whether the deployment method is the user's to pick for a given game. It is a choice only
    /// where both candidates are open.
    /// </summary>
    /// <remarks>
    /// The host has to be able to inject, which means Windows; elsewhere the proxy is the only way
    /// in whatever the user would prefer. The game also has to be one the host default applies to.
    /// A native build settles the question itself by naming the patcher in its own dependency
    /// list, and no preference changes that. A null game is treated as a choice, since nothing
    /// about an unrecognised executable rules the host default out.
    /// </remarks>
    public static bool HasDeploymentChoice(GameVersion? gameVersion)
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return false;
        }

        return gameVersion is null || gameVersion.Platform == Platform.Windows;
    }

    /// <summary>
    /// The deployment method for a specific detected game. A game whose executable can name the
    /// patcher in its own dependency list uses <see cref="DeploymentMethod.LinkedDependency"/>.
    /// Everything else is a Windows PE and falls back to the host default, which is the proxy
    /// under Wine or Proton and injection on Windows.
    /// </summary>
    public static DeploymentMethod ForGame(GameVersion gameVersion)
    {
        return gameVersion.Platform == Platform.Windows
            ? ForCurrentPlatform()
            : DeploymentMethod.LinkedDependency;
    }

    /// <summary>
    /// Whether the manager can start the game executable itself, rather than handing off to Steam
    /// or a user-supplied command.
    /// </summary>
    /// <remarks>
    /// This is a question about the host, not about the deployment method. A Windows host runs a
    /// Windows game directly whatever loads the patcher into it, which is what injection has
    /// always done. Everywhere else the executable is either a Windows build that needs Wine or a
    /// native build Steam owns, so the user's configured launch method is the only way in.
    /// </remarks>
    public static bool CanStartGameDirectly()
    {
        return RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
    }

    /// <summary>
    /// Whether the user has to tell the manager how the game is started. A native build is
    /// reached through Steam and nothing else, so there is nothing to configure. A Windows build
    /// on a host that cannot run it needs a compatibility layer the manager knows nothing about,
    /// whether that is Proton through Steam or a Wine command the user supplies, so it has to
    /// ask. An undetected executable is treated as a Windows build, which is what the rest of the
    /// policy already assumes.
    /// </summary>
    /// <remarks>
    /// This reads like <see cref="HasDeploymentChoice"/> and comes out the same today, but the
    /// questions are different: that one asks how the patcher gets in, this one asks how the game
    /// gets started. Keeping them apart means changing one cannot quietly move the other.
    /// </remarks>
    public static bool NeedsLaunchConfiguration(GameVersion? gameVersion)
    {
        if (CanStartGameDirectly())
        {
            return false;
        }

        return gameVersion is null || gameVersion.Platform == Platform.Windows;
    }

    /// <summary>
    /// The patcher runtime module the game loads. This is the single source of that choice, shared
    /// by install staging and launch, so a new platform adds its module here rather than at each
    /// call site.
    /// </summary>
    /// <remarks>
    /// It follows the game, not the deployment method and not the host: a Windows build loads the
    /// DLL whether that arrives by injection or through the proxy, and it still does when the host
    /// running it is Linux. LinkedDependency alone no longer identifies the module, since both the
    /// Linux and macOS builds reach the patcher that way and load different files.
    /// </remarks>
    public static string PatcherModuleFileName(Platform gamePlatform)
    {
        return gamePlatform switch
        {
            Platform.Linux => "KotorPatcher.so",
            Platform.macOS => "KotorPatcher.dylib",
            _ => "KotorPatcher.dll",
        };
    }

    /// <summary>
    /// The patcher runtime module for a detected game. An unrecognised executable is treated as a
    /// Windows build, which is what the deployment default already assumes.
    /// </summary>
    public static string PatcherModuleFileName(GameVersion? gameVersion) =>
        PatcherModuleFileName(gameVersion?.Platform ?? Platform.Windows);

    /// <summary>
    /// Every patcher module a game can name in its own dependency list. Uninstall works from a
    /// game folder without knowing which platform installed it, so it asks for all of them rather
    /// than guessing. The set follows the same rule <see cref="ForGame"/> uses: a Windows build
    /// goes through the host default, everything else links the module directly.
    /// </summary>
    public static IReadOnlyList<string> LinkedModuleFileNames { get; } =
        Enum.GetValues<Platform>()
            .Where(p => p != Platform.Windows)
            .Select(PatcherModuleFileName)
            .ToArray();

    /// <summary>
    /// The library <see cref="DeploymentMethod.LibraryProxy"/> stands in for. KOTOR 1 and 2 both
    /// import binkw32.dll and Wine has no builtin, so the game's own loader picks up whatever sits
    /// under that name in the game directory.
    /// </summary>
    public const string ProxyLibraryFileName = "binkw32.dll";
}
