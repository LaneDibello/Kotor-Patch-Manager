using System.Diagnostics;

namespace KPatchCore.Launcher;

/// <summary>
/// Result of a game launch operation
/// </summary>
public sealed class LaunchResult
{
    /// <summary>
    /// Whether the launch succeeded
    /// </summary>
    public required bool Success { get; init; }

    /// <summary>
    /// Error message if launch failed
    /// </summary>
    public string? Error { get; init; }

    /// <summary>
    /// The launched game process (if successful)
    /// </summary>
    public Process? GameProcess { get; init; }

    /// <summary>
    /// Process ID of the launched game (convenience property)
    /// </summary>
    public int? ProcessId => GameProcess?.Id;

    /// <summary>
    /// Whether DLL injection was performed
    /// </summary>
    public bool InjectionPerformed { get; init; }

    /// <summary>
    /// Whether this was a vanilla (unpatched) launch
    /// </summary>
    public bool VanillaLaunch { get; init; }

    /// <summary>
    /// Additional informational messages
    /// </summary>
    public List<string> Messages { get; init; } = new();

    /// <summary>
    /// Creates a result for a launch that got the patches into the game.
    /// </summary>
    /// <param name="process">The launched process</param>
    /// <param name="injectionPerformed">Whether the patcher was injected, as opposed to the game
    /// loading it itself</param>
    /// <param name="message">Optional message</param>
    public static LaunchResult Patched(Process process, bool injectionPerformed, string? message = null)
    {
        return Create(process, injectionPerformed, vanillaLaunch: false, message);
    }

    /// <summary>
    /// Creates a result for a launch of a game with no patches installed.
    /// </summary>
    /// <param name="process">The launched process</param>
    /// <param name="message">Optional message</param>
    public static LaunchResult Vanilla(Process process, string? message = null)
    {
        return Create(process, injectionPerformed: false, vanillaLaunch: true, message);
    }

    // Whether the patcher was injected and whether the game is patched are separate facts. They
    // agreed only while injection was the one way patches reached a Windows game; the library
    // proxy has the game load the patcher itself, so a patched launch injects nothing.
    private static LaunchResult Create(
        Process process,
        bool injectionPerformed,
        bool vanillaLaunch,
        string? message)
    {
        var result = new LaunchResult
        {
            Success = true,
            GameProcess = process,
            InjectionPerformed = injectionPerformed,
            VanillaLaunch = vanillaLaunch
        };

        if (message != null)
        {
            result.Messages.Add(message);
        }

        return result;
    }

    /// <summary>
    /// Creates a successful result for a game started through an external
    /// launcher (e.g. Steam), where the manager has no handle to the game
    /// process. Patches load via the staged KProxy, not live injection.
    /// </summary>
    /// <param name="message">What was launched and how</param>
    /// <returns>Successful, process-less launch result</returns>
    public static LaunchResult Launched(string message)
    {
        var result = new LaunchResult
        {
            Success = true,
            GameProcess = null,
            InjectionPerformed = false,
            VanillaLaunch = false
        };
        result.Messages.Add(message);
        return result;
    }

    /// <summary>
    /// Creates a failed launch result
    /// </summary>
    /// <param name="error">Error message</param>
    /// <returns>Failed launch result</returns>
    public static LaunchResult Fail(string error)
    {
        return new LaunchResult
        {
            Success = false,
            Error = error,
            InjectionPerformed = false,
            VanillaLaunch = false
        };
    }

    /// <summary>
    /// Adds an informational message to the result
    /// </summary>
    public LaunchResult WithMessage(string message)
    {
        Messages.Add(message);
        return this;
    }

    /// <summary>
    /// Adds multiple messages to the result
    /// </summary>
    public LaunchResult WithMessages(IEnumerable<string> messages)
    {
        Messages.AddRange(messages);
        return this;
    }

    public override string ToString()
    {
        if (Success)
        {
            var mode = VanillaLaunch ? "vanilla" : "with patches";
            var pidInfo = ProcessId.HasValue ? $" (PID: {ProcessId})" : "";
            return $"Game launched {mode}{pidInfo}";
        }

        return $"Launch failed: {Error}";
    }
}
