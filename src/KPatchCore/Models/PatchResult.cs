namespace KPatchCore.Models;

/// <summary>
/// Result of a patch operation (install, uninstall, validate, etc.)
/// Uses Result pattern instead of exceptions for expected failures
/// </summary>
public class PatchResult
{
    /// <summary>
    /// Whether the operation succeeded
    /// </summary>
    public bool Success { get; init; }

    /// <summary>
    /// Error message if operation failed
    /// </summary>
    public string? Error { get; init; }

    /// <summary>
    /// Additional informational messages
    /// </summary>
    public List<string> Messages { get; init; } = new();

    /// <summary>
    /// Data returned by the operation (if any)
    /// </summary>
    public object? Data { get; init; }

    /// <summary>
    /// Creates a successful result
    /// </summary>
    public static PatchResult Ok(string? message = null, object? data = null)
    {
        var result = new PatchResult
        {
            Success = true,
            Data = data
        };

        if (message != null)
            result.Messages.Add(message);

        return result;
    }

    /// <summary>
    /// Creates a failed result
    /// </summary>
    public static PatchResult Fail(string error)
    {
        return new PatchResult
        {
            Success = false,
            Error = error
        };
    }

    /// <summary>
    /// Adds an informational message to the result
    /// </summary>
    public PatchResult WithMessage(string message)
    {
        Messages.Add(message);
        return this;
    }

    /// <summary>
    /// Adds multiple messages to the result
    /// </summary>
    public PatchResult WithMessages(IEnumerable<string> messages)
    {
        Messages.AddRange(messages);
        return this;
    }

    public override string ToString()
    {
        if (Success)
        {
            return Messages.Count > 0
                ? $"Success: {string.Join("; ", Messages)}"
                : "Success";
        }

        return $"Error: {Error}";
    }
}

/// <summary>
/// Strongly-typed result for operations that return data
/// </summary>
public sealed class PatchResult<T> : PatchResult
{
    /// <summary>
    /// Strongly-typed data
    /// </summary>
    public new T? Data
    {
        get => (T?)base.Data;
        init => base.Data = value;
    }

    /// <summary>
    /// Creates a successful result with data
    /// </summary>
    public static PatchResult<T> Ok(T data, string? message = null)
    {
        var result = new PatchResult<T>
        {
            Success = true,
            Data = data
        };

        if (message != null)
            result.Messages.Add(message);

        return result;
    }

    /// <summary>
    /// Creates a failed result
    /// </summary>
    public new static PatchResult<T> Fail(string error)
    {
        return new PatchResult<T>
        {
            Success = false,
            Error = error
        };
    }
}
