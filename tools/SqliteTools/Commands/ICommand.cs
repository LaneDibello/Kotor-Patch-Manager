namespace SqliteTools.Commands;

public interface ICommand
{
    void Execute(string[] args);
}
