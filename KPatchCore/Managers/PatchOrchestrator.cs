using KPatchCore.Applicators;
using KPatchCore.Models;

namespace KPatchCore.Managers;

/// <summary>
/// Main public API facade for the patch management system
/// </summary>
/// <remarks>
/// This is the primary entry point for consumers of KPatchCore.
/// It provides a simple, high-level API for installing and removing patches.
/// </remarks>
public class PatchOrchestrator
{
    private readonly PatchRepository _repository;
    private readonly PatchApplicator _applicator;

    /// <summary>
    /// Creates a new patch orchestrator
    /// </summary>
    /// <param name="patchesDirectory">Directory containing .kpatch files</param>
    public PatchOrchestrator(string patchesDirectory)
    {
        _repository = new PatchRepository(patchesDirectory);
        _applicator = new PatchApplicator(_repository);

        // Automatically scan for patches on initialization
        var scanResult = _repository.ScanPatches();
        if (!scanResult.Success)
        {
            // Log warning but don't fail construction
            // Consumer can check GetAvailablePatches() to see if any loaded
        }
    }

    /// <summary>
    /// Installs patches to a game
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="patchIds">Patch IDs to install</param>
    /// <param name="createBackup">Whether to create a backup before installation</param>
    /// <param name="injectLoader">Whether to inject the loader DLL</param>
    /// <param name="patcherDllPath">Path to KotorPatcher.dll (optional)</param>
    /// <returns>Installation result</returns>
    public PatchApplicator.InstallResult InstallPatches(
        string gameExePath,
        IEnumerable<string> patchIds,
        bool createBackup = true,
        bool injectLoader = true,
        string? patcherDllPath = null)
    {
        var options = new PatchApplicator.InstallOptions
        {
            GameExePath = gameExePath,
            PatchIds = patchIds.ToList(),
            CreateBackup = createBackup,
            InjectLoader = injectLoader,
            PatcherDllPath = patcherDllPath
        };

        return _applicator.InstallPatches(options);
    }

    /// <summary>
    /// Removes all patches from a game installation
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <returns>Removal result</returns>
    public PatchRemover.RemovalResult UninstallPatches(string gameExePath)
    {
        return PatchRemover.RemoveAllPatches(gameExePath);
    }

    /// <summary>
    /// Removes specific patches (not supported in MVP - removes all)
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <param name="patchIds">Patch IDs to remove</param>
    /// <returns>Removal result</returns>
    public PatchRemover.RemovalResult UninstallPatches(string gameExePath, IEnumerable<string> patchIds)
    {
        return PatchRemover.RemovePatches(gameExePath, patchIds.ToList());
    }

    /// <summary>
    /// Gets all available patches in the repository
    /// </summary>
    /// <returns>Dictionary of patch ID to PatchEntry</returns>
    public IReadOnlyDictionary<string, PatchRepository.PatchEntry> GetAvailablePatches()
    {
        return _repository.GetAllPatches();
    }

    /// <summary>
    /// Gets a specific patch by ID
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <returns>Result containing PatchEntry or error</returns>
    public PatchResult<PatchRepository.PatchEntry> GetPatch(string patchId)
    {
        return _repository.GetPatch(patchId);
    }

    /// <summary>
    /// Checks if a game has patches installed
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <returns>Result indicating if patches are installed</returns>
    public PatchResult<bool> IsPatched(string gameExePath)
    {
        return PatchRemover.HasPatchesInstalled(gameExePath);
    }

    /// <summary>
    /// Gets detailed information about a game's patch installation
    /// </summary>
    /// <param name="gameExePath">Path to the game executable</param>
    /// <returns>Result containing installation info</returns>
    public PatchResult<PatchRemover.InstallationInfo> GetInstallationInfo(string gameExePath)
    {
        return PatchRemover.GetInstallationInfo(gameExePath);
    }

    /// <summary>
    /// Rescans the patches directory for new .kpatch files
    /// </summary>
    /// <returns>Result indicating success or failure</returns>
    public PatchResult RescanPatches()
    {
        return _repository.ScanPatches();
    }

    /// <summary>
    /// Gets the number of available patches
    /// </summary>
    public int AvailablePatchCount => _repository.PatchCount;

    /// <summary>
    /// Checks if a specific patch is available
    /// </summary>
    /// <param name="patchId">Patch ID</param>
    /// <returns>True if patch is available, false otherwise</returns>
    public bool HasPatch(string patchId) => _repository.HasPatch(patchId);

    /// <summary>
    /// Finds patches that match specific criteria
    /// </summary>
    /// <param name="predicate">Filter function</param>
    /// <returns>List of matching patches</returns>
    public List<PatchRepository.PatchEntry> FindPatches(Func<PatchRepository.PatchEntry, bool> predicate)
    {
        return _repository.FindPatches(predicate);
    }
}
