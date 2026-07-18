using KPatchCore.Models;

namespace KPatchCore.Applicators;

/// <summary>
/// Stages the KProxy that loads KotorPatcher.dll when the game starts, for
/// the proxy deployment method (see <see cref="Launcher.DeploymentPolicy"/>).
/// </summary>
/// <remarks>
/// The game's real binkw32.dll is renamed to binkw32Hooked.dll (which the proxy
/// forwards Bink calls to) and the proxy takes its place as binkw32.dll. When the
/// game starts, its loader pulls in the proxy, which loads KotorPatcher.dll. This
/// is mechanical only; whether it runs is decided by the deployment policy.
/// </remarks>
public static class KProxyInstaller
{
    private const string BinkDll = "binkw32.dll";
    private const string BinkHookedDll = "binkw32Hooked.dll";

    /// <summary>
    /// Installs the proxy into a game directory: renames the original Bink to
    /// binkw32Hooked.dll (first install only) and copies the proxy in as binkw32.dll.
    /// </summary>
    /// <param name="gameDir">Directory containing the game's binkw32.dll.</param>
    /// <param name="proxyDllPath">Path to the built proxy to stage.</param>
    /// <returns>Success, or failure if the proxy or the game's Bink is missing.</returns>
    public static PatchResult Install(string gameDir, string proxyDllPath)
    {
        if (!File.Exists(proxyDllPath))
        {
            return PatchResult.Fail($"KProxy not found at: {proxyDllPath}");
        }

        var bink = Path.Combine(gameDir, BinkDll);
        var hooked = Path.Combine(gameDir, BinkHookedDll);

        // Preserve the real Bink for forwarding, but only the first time: on a
        // reapply, binkw32.dll is already our proxy, so moving it would overwrite
        // the saved original with the proxy.
        if (!File.Exists(hooked))
        {
            if (!File.Exists(bink))
            {
                return PatchResult.Fail(
                    $"Game's binkw32.dll not found in {gameDir}; cannot install proxy");
            }

            File.Move(bink, hooked);
        }

        File.Copy(proxyDllPath, bink, overwrite: true);
        return PatchResult.Ok("Installed KProxy (binkw32.dll loads KotorPatcher)");
    }

    /// <summary>
    /// Reverses <see cref="Install"/>: removes the proxy and restores the original
    /// binkw32.dll. A no-op if the proxy was never installed.
    /// </summary>
    /// <param name="gameDir">Directory the proxy was installed into.</param>
    public static PatchResult Uninstall(string gameDir)
    {
        var bink = Path.Combine(gameDir, BinkDll);
        var hooked = Path.Combine(gameDir, BinkHookedDll);

        if (!File.Exists(hooked))
        {
            return PatchResult.Ok();
        }

        // binkw32.dll is currently our proxy; drop it and move the real Bink back.
        if (File.Exists(bink))
        {
            File.Delete(bink);
        }

        File.Move(hooked, bink);
        return PatchResult.Ok("Removed KProxy, restored original binkw32.dll");
    }
}
