namespace SqliteTools.Models;

public class GhidraOffset
{
    public string ClassName { get; set; } = string.Empty;
    public string MemberName { get; set; } = string.Empty;
    public int Offset { get; set; }
    public string? Notes { get; set; }
}
