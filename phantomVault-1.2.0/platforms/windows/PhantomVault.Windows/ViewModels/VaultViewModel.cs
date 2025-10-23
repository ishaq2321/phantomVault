using CommunityToolkit.Mvvm.ComponentModel;
using PhantomVault.Windows.Models;

namespace PhantomVault.Windows.ViewModels;

public partial class VaultViewModel : ObservableObject
{
    private readonly Vault _vault;

    public VaultViewModel(Vault vault)
    {
        _vault = vault;
    }

    public string Id => _vault.Id;
    
    public string Name
    {
        get => _vault.Name;
        set
        {
            _vault.Name = value;
            OnPropertyChanged();
        }
    }

    public string Description
    {
        get => _vault.Description;
        set
        {
            _vault.Description = value;
            OnPropertyChanged();
        }
    }

    public string Location => _vault.Location;

    public DateTime CreatedTime => _vault.CreatedTime;

    public DateTime LastModified
    {
        get => _vault.ModifiedTime;
        set
        {
            _vault.ModifiedTime = value;
            OnPropertyChanged();
        }
    }

    [ObservableProperty]
    private bool _isLocked = true;

    [ObservableProperty]
    private bool _isSecure = true;

    [ObservableProperty]
    private long _size;

    [ObservableProperty]
    private string _status = "Locked";

    public string SizeFormatted => FormatSize(Size);

    private static string FormatSize(long bytes)
    {
        string[] sizes = { "B", "KB", "MB", "GB", "TB" };
        double len = bytes;
        int order = 0;
        while (len >= 1024 && order < sizes.Length - 1)
        {
            order++;
            len = len / 1024;
        }
        return $"{len:0.##} {sizes[order]}";
    }
}
