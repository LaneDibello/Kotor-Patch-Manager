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
    /// Creates a successful launch result
    /// </summary>
    /// <param name="process">The launched process</param>
    /// <param name="injectionPerformed">Whether DLL injection was performed</param>
    /// <param name="message">Optional message</param>
    /// <returns>Successful launch result</returns>
    public static LaunchResult Ok(Process process, bool injectionPerformed, string? message = null)
    {
        var result = new LaunchResult
        {
            Success = true,
            GameProcess = process,
            InjectionPerformed = injectionPerformed,
            VanillaLaunch = !injectionPerformed
        };

        if (message != null)
        {
            result.Messages.Add(message);
        }

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
