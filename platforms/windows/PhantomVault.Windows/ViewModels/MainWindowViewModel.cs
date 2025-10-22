using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using PhantomVault.Windows.Models;
using PhantomVault.Windows.Services;
using System.Collections.ObjectModel;
using System.Windows;

namespace PhantomVault.Windows.ViewModels;

public partial class MainWindowViewModel : ObservableObject
{
    private readonly IVaultService _vaultService;
    private readonly ISystemTrayService _systemTrayService;
    private readonly IKeyboardHookService _keyboardHookService;
    private readonly INotificationService _notificationService;

    [ObservableProperty]
    private ObservableCollection<VaultViewModel> _vaults = new();

    [ObservableProperty]
    private object? _currentView;

    [ObservableProperty]
    private string _statusText = "Secure";

    [ObservableProperty]
    private string _statusColor = "Green";

    [ObservableProperty]
    private string _statusMessage = "Ready";

    [ObservableProperty]
    private int _vaultCount;

    [ObservableProperty]
    private string _version = "1.0.0";

    [ObservableProperty]
    private bool _keepInTray = true;

    public MainWindowViewModel(
        IVaultService vaultService,
        ISystemTrayService systemTrayService,
        IKeyboardHookService keyboardHookService,
        INotificationService notificationService)
    {
        _vaultService = vaultService;
        _systemTrayService = systemTrayService;
        _keyboardHookService = keyboardHookService;
        _notificationService = notificationService;

        // Initialize with dashboard view
        CurrentView = new DashboardViewModel(_vaultService, _notificationService);
        
        // Load vaults
        LoadVaults();
        
        // Set up system tray
        SetupSystemTray();
    }

    [RelayCommand]
    private async Task AddVault()
    {
        try
        {
            StatusMessage = "Creating new vault...";
            
            // Show setup wizard
            var setupWizard = new SetupWizardWindow();
            if (setupWizard.ShowDialog() == true)
            {
                await LoadVaults();
                StatusMessage = "Vault created successfully";
                _notificationService.ShowSuccess("Vault created successfully");
            }
        }
        catch (Exception ex)
        {
            StatusMessage = "Error creating vault";
            _notificationService.ShowError($"Failed to create vault: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task ImportVault()
    {
        try
        {
            StatusMessage = "Importing vault...";
            
            // TODO: Implement vault import functionality
            _notificationService.ShowInfo("Vault import feature coming soon");
            StatusMessage = "Ready";
        }
        catch (Exception ex)
        {
            StatusMessage = "Error importing vault";
            _notificationService.ShowError($"Failed to import vault: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task BackupAll()
    {
        try
        {
            StatusMessage = "Backing up all vaults...";
            
            // TODO: Implement backup functionality
            _notificationService.ShowInfo("Backup feature coming soon");
            StatusMessage = "Ready";
        }
        catch (Exception ex)
        {
            StatusMessage = "Error backing up vaults";
            _notificationService.ShowError($"Failed to backup vaults: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task UnlockVault(VaultViewModel vault)
    {
        try
        {
            StatusMessage = $"Unlocking vault: {vault.Name}";
            
            // TODO: Implement vault unlocking
            vault.IsLocked = false;
            StatusMessage = $"Vault '{vault.Name}' unlocked";
            _notificationService.ShowSuccess($"Vault '{vault.Name}' unlocked");
        }
        catch (Exception ex)
        {
            StatusMessage = "Error unlocking vault";
            _notificationService.ShowError($"Failed to unlock vault: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task LockVault(VaultViewModel vault)
    {
        try
        {
            StatusMessage = $"Locking vault: {vault.Name}";
            
            // TODO: Implement vault locking
            vault.IsLocked = true;
            StatusMessage = $"Vault '{vault.Name}' locked";
            _notificationService.ShowSuccess($"Vault '{vault.Name}' locked");
        }
        catch (Exception ex)
        {
            StatusMessage = "Error locking vault";
            _notificationService.ShowError($"Failed to lock vault: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task DeleteVault(VaultViewModel vault)
    {
        try
        {
            var result = MessageBox.Show(
                $"Are you sure you want to delete the vault '{vault.Name}'? This action cannot be undone.",
                "Confirm Delete",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning);

            if (result == MessageBoxResult.Yes)
            {
                StatusMessage = $"Deleting vault: {vault.Name}";
                
                // TODO: Implement vault deletion
                Vaults.Remove(vault);
                VaultCount = Vaults.Count;
                StatusMessage = $"Vault '{vault.Name}' deleted";
                _notificationService.ShowSuccess($"Vault '{vault.Name}' deleted");
            }
        }
        catch (Exception ex)
        {
            StatusMessage = "Error deleting vault";
            _notificationService.ShowError($"Failed to delete vault: {ex.Message}");
        }
    }

    [RelayCommand]
    private void OpenSettings()
    {
        try
        {
            var settingsWindow = new SettingsWindow();
            settingsWindow.ShowDialog();
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to open settings: {ex.Message}");
        }
    }

    [RelayCommand]
    private void MinimizeToTray()
    {
        Application.Current.MainWindow!.WindowState = WindowState.Minimized;
    }

    [RelayCommand]
    private void Exit()
    {
        Application.Current.Shutdown();
    }

    public void InitializeKeyboardHooks()
    {
        try
        {
            // Set up global keyboard shortcuts
            _keyboardHookService.RegisterHotKey(ModifierKeys.Control | ModifierKeys.Alt, Key.V, () =>
            {
                // Show/hide main window
                var mainWindow = Application.Current.MainWindow;
                if (mainWindow != null)
                {
                    if (mainWindow.Visibility == Visibility.Visible)
                    {
                        mainWindow.Hide();
                    }
                    else
                    {
                        mainWindow.Show();
                        mainWindow.Activate();
                    }
                }
            });

            _keyboardHookService.RegisterHotKey(ModifierKeys.Control | ModifierKeys.Alt, Key.L, () =>
            {
                // Lock all vaults
                foreach (var vault in Vaults)
                {
                    vault.IsLocked = true;
                }
                _notificationService.ShowInfo("All vaults locked");
            });
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to initialize keyboard hooks: {ex.Message}");
        }
    }

    private async Task LoadVaults()
    {
        try
        {
            var vaults = await _vaultService.GetAllVaultsAsync();
            Vaults.Clear();
            
            foreach (var vault in vaults)
            {
                Vaults.Add(new VaultViewModel(vault));
            }
            
            VaultCount = Vaults.Count;
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to load vaults: {ex.Message}");
        }
    }

    private void SetupSystemTray()
    {
        try
        {
            _systemTrayService.Initialize();
            _systemTrayService.SetIcon("Resources/icon.ico");
            _systemTrayService.SetTooltip("Phantom Vault - Secure Folder Management");
            
            _systemTrayService.AddMenuItem("Show Phantom Vault", () =>
            {
                Application.Current.MainWindow!.Show();
                Application.Current.MainWindow.Activate();
            });
            
            _systemTrayService.AddMenuItem("Lock All Vaults", () =>
            {
                foreach (var vault in Vaults)
                {
                    vault.IsLocked = true;
                }
                _notificationService.ShowInfo("All vaults locked");
            });
            
            _systemTrayService.AddMenuItem("Exit", () =>
            {
                Application.Current.Shutdown();
            });
        }
        catch (Exception ex)
        {
            _notificationService.ShowError($"Failed to setup system tray: {ex.Message}");
        }
    }
}
