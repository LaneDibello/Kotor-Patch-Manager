using Xunit;

using KPatchCore.Launcher;
using KPatchCore.Managers;
using KPatchCore.Models;

namespace KPatchCore.Tests;

/// <summary>
/// A game keeps the deployment its last install recorded, in either direction; an explicit
/// request outranks it; with neither, the preference decides.
/// </summary>
/// <remarks>
/// Every case sets whether the host can inject, so the Windows rules run on any CI host. The
/// policy's settings are process-wide, so each case puts them back. Nothing else in this assembly
/// touches them.
/// </remarks>
public class DeploymentPolicyTests
{
    private static GameVersion Game(Platform platform) => new()
    {
        Platform = platform,
        Distribution = Distribution.GOG,
        Version = "1.0.3",
        Architecture = Architecture.x86,
        Title = GameTitle.KOTOR1,
        FileSize = 4042752,
        Hash = "761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886",
    };

    private static ManagedInstallState Installed(GameVersion game, bool libraryProxy) => new()
    {
        GameExePath = "swkotor.exe",
        GameExeFileName = "swkotor.exe",
        OriginalHash = game.Hash,
        OriginalFileSize = game.FileSize,
        OriginalVersion = game,
        LibraryProxyInstalled = libraryProxy,
        CreatedAt = DateTime.UtcNow,
        UpdatedAt = DateTime.UtcNow,
    };

    private static T With<T>(bool hostCanInject, bool preferLibraryProxy, DeploymentMethod? requested, Func<T> body)
    {
        var saved = (DeploymentPolicy.HostCanInjectOverride, DeploymentPolicy.PreferLibraryProxy,
            DeploymentPolicy.RequestedDeployment);
        DeploymentPolicy.HostCanInjectOverride = hostCanInject;
        DeploymentPolicy.PreferLibraryProxy = preferLibraryProxy;
        DeploymentPolicy.RequestedDeployment = requested;
        try
        {
            return body();
        }
        finally
        {
            (DeploymentPolicy.HostCanInjectOverride, DeploymentPolicy.PreferLibraryProxy,
                DeploymentPolicy.RequestedDeployment) = saved;
        }
    }

    private static DeploymentMethod Decide(bool hostCanInject, bool preferLibraryProxy, bool? installedWithProxy,
        DeploymentMethod? requested = null)
    {
        var game = Game(Platform.Windows);
        var installed = installedWithProxy is bool proxy ? Installed(game, proxy) : null;
        return With(hostCanInject, preferLibraryProxy, requested, () => DeploymentPolicy.ForGame(game, installed));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void AProxyInstallKeepsTheProxy(bool preferLibraryProxy)
    {
        Assert.Equal(DeploymentMethod.LibraryProxy, Decide(true, preferLibraryProxy, installedWithProxy: true));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void AnInjectionInstallKeepsInjection(bool preferLibraryProxy)
    {
        Assert.Equal(DeploymentMethod.RuntimeInjection, Decide(true, preferLibraryProxy, installedWithProxy: false));
    }

    [Theory]
    [InlineData(false, DeploymentMethod.RuntimeInjection)]
    [InlineData(true, DeploymentMethod.LibraryProxy)]
    public void WithNothingInstalledThePreferenceDecides(bool preferLibraryProxy, DeploymentMethod expected)
    {
        Assert.Equal(expected, Decide(true, preferLibraryProxy, installedWithProxy: null));
    }

    [Theory]
    [InlineData(null)]
    [InlineData(false)]
    [InlineData(true)]
    public void AHostThatCannotInjectStaysOnTheProxy(bool? installedWithProxy)
    {
        Assert.Equal(DeploymentMethod.LibraryProxy, Decide(false, false, installedWithProxy));
        Assert.Equal(DeploymentMethod.LibraryProxy,
            Decide(false, false, installedWithProxy, DeploymentMethod.RuntimeInjection));
    }

    [Theory]
    [InlineData(DeploymentMethod.RuntimeInjection)]
    [InlineData(DeploymentMethod.LibraryProxy)]
    public void AnExplicitRequestOutranksTheInstallAndThePreference(DeploymentMethod requested)
    {
        // The install and the preference both name the other method.
        var other = requested != DeploymentMethod.LibraryProxy;
        Assert.Equal(requested, Decide(true, other, other, requested));
    }

    [Theory]
    [InlineData(Platform.macOS)]
    [InlineData(Platform.Linux)]
    public void ANativeBuildIsUnaffected(Platform platform)
    {
        var game = Game(platform);
        foreach (var proxy in new[] { false, true })
        {
            var method = With(true, !proxy, DeploymentMethod.RuntimeInjection,
                () => DeploymentPolicy.ForGame(game, Installed(game, proxy)));
            Assert.Equal(DeploymentMethod.LinkedDependency, method);
        }
    }

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    public void NoGameRecordsNoInstall(string? gameExePath)
    {
        Assert.Null(DeploymentPolicy.InstalledWithLibraryProxy(gameExePath));
    }

    // A game folder with binkw32.dll as it is after an install ("staged": the proxy in front of
    // the saved original), after Steam's file check put the stock file back ("verified": both
    // the same), or never proxied ("stock").
    private static string GameFolder(DirectoryInfo folder, string bink)
    {
        var stock = new byte[] { 1, 2, 3, 4 };
        File.WriteAllBytes(Path.Combine(folder.FullName, "binkw32.dll"),
            bink == "staged" ? new byte[] { 9, 9, 9 } : stock);
        if (bink != "stock")
        {
            File.WriteAllBytes(Path.Combine(folder.FullName, "binkw32Hooked.dll"), stock);
        }

        var exe = Path.Combine(folder.FullName, "swkotor.exe");
        File.WriteAllBytes(exe, new byte[] { 0x4D, 0x5A });
        return exe;
    }

    [Theory]
    [InlineData("staged", true)]
    [InlineData("verified", false)]
    [InlineData("stock", false)]
    public void TheProxyIsStagedOnlyInFrontOfTheSavedOriginal(string bink, bool staged)
    {
        var folder = Directory.CreateTempSubdirectory("kpm-deployment-test-");
        try
        {
            GameFolder(folder, bink);
            Assert.Equal(staged, KPatchCore.Applicators.KProxyInstaller.IsStaged(folder.FullName));
        }
        finally
        {
            folder.Delete(recursive: true);
        }
    }

    [Theory]
    [InlineData("staged", true, DeploymentMethod.LibraryProxy)]
    // Steam's file check restored the stock binkw32.dll: inject, so the game still runs patched.
    [InlineData("verified", true, DeploymentMethod.RuntimeInjection)]
    // A host that cannot inject has nothing else to try.
    [InlineData("verified", false, DeploymentMethod.LibraryProxy)]
    public void LaunchInjectsWhereTheRecordedProxyIsGone(string bink, bool hostCanInject, DeploymentMethod expected)
    {
        var folder = Directory.CreateTempSubdirectory("kpm-deployment-test-");
        try
        {
            var exe = GameFolder(folder, bink);
            var game = Game(Platform.Windows);
            Assert.True(InstallStateManager.SaveOrUpdate(exe, game, new[] { "some-patch" },
                libraryProxyInstalled: true).Success);
            Assert.Equal(expected, With(hostCanInject, true, null, () => DeploymentPolicy.ForLaunch(game, exe)));
            // Apply is unaffected: it stages the proxy again.
            Assert.Equal(DeploymentMethod.LibraryProxy,
                With(hostCanInject, true, null, () => DeploymentPolicy.ForInstalledGame(game, exe)));
        }
        finally
        {
            folder.Delete(recursive: true);
        }
    }

    [Fact]
    public void ForInstalledGameReadsTheStateBesideTheExecutable()
    {
        var folder = Directory.CreateTempSubdirectory("kpm-deployment-test-");
        try
        {
            var exe = Path.Combine(folder.FullName, "swkotor.exe");
            File.WriteAllBytes(exe, new byte[] { 0x4D, 0x5A });
            var game = Game(Platform.Windows);
            var statePath = Path.Combine(folder.FullName, InstallStateManager.StateFileName);

            With(true, false, null, () =>
            {
                Assert.Null(DeploymentPolicy.InstalledWithLibraryProxy(exe));
                Assert.Equal(DeploymentMethod.RuntimeInjection, DeploymentPolicy.ForInstalledGame(game, exe));

                Assert.True(InstallStateManager.SaveOrUpdate(exe, game, new[] { "some-patch" },
                    libraryProxyInstalled: true).Success);
                Assert.True(DeploymentPolicy.InstalledWithLibraryProxy(exe));
                Assert.Equal(DeploymentMethod.LibraryProxy, DeploymentPolicy.ForInstalledGame(game, exe));

                // Uninstall All deletes the state, and the preference decides again.
                File.Delete(statePath);
                Assert.Null(DeploymentPolicy.InstalledWithLibraryProxy(exe));
                Assert.Equal(DeploymentMethod.RuntimeInjection, DeploymentPolicy.ForInstalledGame(game, exe));
                return 0;
            });

            // The same the other way: an injection install stays on injection with the preference on.
            With(true, true, null, () =>
            {
                Assert.True(InstallStateManager.SaveOrUpdate(exe, game, new[] { "some-patch" },
                    libraryProxyInstalled: false).Success);
                Assert.False(DeploymentPolicy.InstalledWithLibraryProxy(exe));
                Assert.Equal(DeploymentMethod.RuntimeInjection, DeploymentPolicy.ForInstalledGame(game, exe));
                return 0;
            });
        }
        finally
        {
            folder.Delete(recursive: true);
        }
    }
}
