namespace SqliteTools.Models;

public class GhidraFunction
{
    public string ClassName { get; set; } = string.Empty;
    public string FunctionName { get; set; } = string.Empty;
    public string Address { get; set; } = string.Empty;
    public string? CallingConvention { get; set; }
    public int? ParamSizeBytes { get; set; }
    public string? Notes { get; set; }
}
