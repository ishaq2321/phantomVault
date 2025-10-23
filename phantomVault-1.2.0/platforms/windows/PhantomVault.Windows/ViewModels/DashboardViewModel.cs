using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using PhantomVault.Windows.Services;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace PhantomVault.Windows.ViewModels;

public partial class DashboardViewModel : ObservableObject
{
    private readonly IVaultService _vaultService;
    private readonly INotificationService _notificationService;

    [ObservableProperty]
    private ObservableCollection<VaultViewModel> _recentVaults = new();

    [ObservableProperty]
    private int _totalVaults;

    [ObservableProperty]
    private long _totalSize;

    [ObservableProperty]
    private int _lockedVaults;

    [ObservableProperty]
    private int _unlockedVaults;

    [ObservableProperty]
    private string _lastActivity = "No recent activity";

    public DashboardViewModel(IVaultService vaultService, INotificationService notificationService)
    {
        _vaultService = vaultService;
        _notificationService = notificationService;
        
        LoadDashboardData();
    }

    [RelayCommand]
    private async Task RefreshDashboard()
    {
        await LoadDashboardData();
        _notificationService.ShowInfo("Dashboard refreshed");
    }

    [RelayCommand]
    private async Task QuickLockAll()
    {
        try
        {
            var vaults = await _vaultService.GetAllVaultsAsync();
            foreach (var vault in vaults)
            {
                await _vaultService.LockVaultAsync(vault.Id);
            }
            
            await LoadDashboardData();
            _notificationService.ShowSuccess("All vaults locked");
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to lock all vaults: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task QuickUnlockAll()
    {
        try
        {
            var vaults = await _vaultService.GetAllVaultsAsync();
            foreach (var vault in vaults)
            {
                if (vault.IsLocked)
                {
                    // TODO: Implement password prompt for unlocking
                    _notificationService.ShowInfo($"Vault '{vault.Name}' requires password to unlock");
                }
            }
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to unlock vaults: {ex.Message}");
        }
    }

    private async Task LoadDashboardData()
    {
        try
        {
            var vaults = await _vaultService.GetAllVaultsAsync();
            
            TotalVaults = 0;
            TotalSize = 0;
            LockedVaults = 0;
            UnlockedVaults = 0;
            
            RecentVaults.Clear();
            
            foreach (var vault in vaults)
            {
                TotalVaults++;
                TotalSize += vault.Size;
                
                if (vault.IsLocked)
                    LockedVaults++;
                else
                    UnlockedVaults++;
                
                // Add to recent vaults (limit to 5)
                if (RecentVaults.Count < 5)
                {
                    RecentVaults.Add(new VaultViewModel(vault));
                }
            }
            
            LastActivity = $"Last updated: {DateTime.Now:MMM dd, yyyy HH:mm}";
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to load dashboard data: {ex.Message}");
        }
    }
}
