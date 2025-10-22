using System;
using System.IO;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public class FileSystemService : IFileSystemService
{
    public async Task<bool> CreateDirectoryAsync(string path)
    {
        try
        {
            await Task.Run(() => Directory.CreateDirectory(path));
            return true;
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> DeleteDirectoryAsync(string path, bool recursive = false)
    {
        try
        {
            await Task.Run(() => Directory.Delete(path, recursive));
            return true;
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> HideFileAsync(string path)
    {
        try
        {
            return await Task.Run(() =>
            {
                if (File.Exists(path))
                {
                    var attributes = File.GetAttributes(path);
                    File.SetAttributes(path, attributes | System.IO.FileAttributes.Hidden);
                    return true;
                }
                return false;
            });
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> UnhideFileAsync(string path)
    {
        try
        {
            return await Task.Run(() =>
            {
                if (File.Exists(path))
                {
                    var attributes = File.GetAttributes(path);
                    File.SetAttributes(path, attributes & ~System.IO.FileAttributes.Hidden);
                    return true;
                }
                return false;
            });
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> IsFileHiddenAsync(string path)
    {
        try
        {
            return await Task.Run(() =>
            {
                if (File.Exists(path))
                {
                    var attributes = File.GetAttributes(path);
                    return (attributes & System.IO.FileAttributes.Hidden) == System.IO.FileAttributes.Hidden;
                }
                return false;
            });
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> FileExistsAsync(string path)
    {
        return await Task.Run(() => File.Exists(path));
    }

    public async Task<bool> DirectoryExistsAsync(string path)
    {
        return await Task.Run(() => Directory.Exists(path));
    }

    public async Task<long> GetFileSizeAsync(string path)
    {
        try
        {
            return await Task.Run(() =>
            {
                var fileInfo = new FileInfo(path);
                return fileInfo.Length;
            });
        }
        catch
        {
            return 0;
        }
    }

    public async Task<DateTime> GetFileModifiedTimeAsync(string path)
    {
        try
        {
            return await Task.Run(() =>
            {
                var fileInfo = new FileInfo(path);
                return fileInfo.LastWriteTime;
            });
        }
        catch
        {
            return DateTime.MinValue;
        }
    }

    public async Task<bool> SetFileAttributesAsync(string path, FileAttributes attributes)
    {
        try
        {
            return await Task.Run(() =>
            {
                if (File.Exists(path))
                {
                    var systemAttributes = ConvertToSystemFileAttributes(attributes);
                    File.SetAttributes(path, systemAttributes);
                    return true;
                }
                return false;
            });
        }
        catch
        {
            return false;
        }
    }

    public async Task<FileAttributes> GetFileAttributesAsync(string path)
    {
        try
        {
            return await Task.Run(() =>
            {
                if (File.Exists(path))
                {
                    var systemAttributes = File.GetAttributes(path);
                    return ConvertFromSystemFileAttributes(systemAttributes);
                }
                return FileAttributes.None;
            });
        }
        catch
        {
            return FileAttributes.None;
        }
    }

    private static System.IO.FileAttributes ConvertToSystemFileAttributes(FileAttributes attributes)
    {
        var systemAttributes = System.IO.FileAttributes.Normal;
        
        if ((attributes & FileAttributes.Hidden) == FileAttributes.Hidden)
            systemAttributes |= System.IO.FileAttributes.Hidden;
        if ((attributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
            systemAttributes |= System.IO.FileAttributes.ReadOnly;
        if ((attributes & FileAttributes.System) == FileAttributes.System)
            systemAttributes |= System.IO.FileAttributes.System;
        if ((attributes & FileAttributes.Archive) == FileAttributes.Archive)
            systemAttributes |= System.IO.FileAttributes.Archive;
            
        return systemAttributes;
    }

    private static FileAttributes ConvertFromSystemFileAttributes(System.IO.FileAttributes systemAttributes)
    {
        var attributes = FileAttributes.None;
        
        if ((systemAttributes & System.IO.FileAttributes.Hidden) == System.IO.FileAttributes.Hidden)
            attributes |= FileAttributes.Hidden;
        if ((systemAttributes & System.IO.FileAttributes.ReadOnly) == System.IO.FileAttributes.ReadOnly)
            attributes |= FileAttributes.ReadOnly;
        if ((systemAttributes & System.IO.FileAttributes.System) == System.IO.FileAttributes.System)
            attributes |= FileAttributes.System;
        if ((systemAttributes & System.IO.FileAttributes.Archive) == System.IO.FileAttributes.Archive)
            attributes |= FileAttributes.Archive;
            
        return attributes;
    }
}
