using System;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public interface IFileSystemService
{
    Task<bool> CreateDirectoryAsync(string path);
    Task<bool> DeleteDirectoryAsync(string path, bool recursive = false);
    Task<bool> HideFileAsync(string path);
    Task<bool> UnhideFileAsync(string path);
    Task<bool> IsFileHiddenAsync(string path);
    Task<bool> FileExistsAsync(string path);
    Task<bool> DirectoryExistsAsync(string path);
    Task<long> GetFileSizeAsync(string path);
    Task<DateTime> GetFileModifiedTimeAsync(string path);
    Task<bool> SetFileAttributesAsync(string path, FileAttributes attributes);
    Task<FileAttributes> GetFileAttributesAsync(string path);
}

[Flags]
public enum FileAttributes
{
    None = 0,
    Hidden = 1,
    ReadOnly = 2,
    System = 4,
    Archive = 8
}
