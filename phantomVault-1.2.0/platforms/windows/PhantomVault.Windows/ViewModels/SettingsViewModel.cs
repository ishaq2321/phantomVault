using CommunityToolkit.Mvvm.ComponentModel;
using PhantomVault.Windows.Services;

namespace PhantomVault.Windows.ViewModels;

public partial class SettingsViewModel : ObservableObject
{
    private readonly IConfigurationService _configurationService;

    [ObservableProperty]
    private bool _autoLock = true;

    [ObservableProperty]
    private int _lockTimeoutMinutes = 30;

    [ObservableProperty]
    private bool _clearClipboard = true;

    [ObservableProperty]
    private int _clipboardTimeoutMinutes = 5;

    [ObservableProperty]
    private bool _hideVaultDirectory = true;

    [ObservableProperty]
    private bool _secureDelete = false;

    [ObservableProperty]
    private int _secureDeletePasses = 3;

    [ObservableProperty]
    private bool _startWithWindows = true;

    [ObservableProperty]
    private bool _minimizeToTray = true;

    [ObservableProperty]
    private bool _showNotifications = true;

    public SettingsViewModel(IConfigurationService configurationService)
    {
        _configurationService = configurationService;
        LoadSettings();
    }

    private async void LoadSettings()
    {
        AutoLock = await _configurationService.GetSettingAsync("AutoLock", true);
        LockTimeoutMinutes = await _configurationService.GetSettingAsync("LockTimeoutMinutes", 30);
        ClearClipboard = await _configurationService.GetSettingAsync("ClearClipboard", true);
        ClipboardTimeoutMinutes = await _configurationService.GetSettingAsync("ClipboardTimeoutMinutes", 5);
        HideVaultDirectory = await _configurationService.GetSettingAsync("HideVaultDirectory", true);
        SecureDelete = await _configurationService.GetSettingAsync("SecureDelete", false);
        SecureDeletePasses = await _configurationService.GetSettingAsync("SecureDeletePasses", 3);
        StartWithWindows = await _configurationService.GetSettingAsync("StartWithWindows", true);
        MinimizeToTray = await _configurationService.GetSettingAsync("MinimizeToTray", true);
        ShowNotifications = await _configurationService.GetSettingAsync("ShowNotifications", true);
    }

    public async Task SaveSettingsAsync()
    {
        await _configurationService.SetSettingAsync("AutoLock", AutoLock);
        await _configurationService.SetSettingAsync("LockTimeoutMinutes", LockTimeoutMinutes);
        await _configurationService.SetSettingAsync("ClearClipboard", ClearClipboard);
        await _configurationService.SetSettingAsync("ClipboardTimeoutMinutes", ClipboardTimeoutMinutes);
        await _configurationService.SetSettingAsync("HideVaultDirectory", HideVaultDirectory);
        await _configurationService.SetSettingAsync("SecureDelete", SecureDelete);
        await _configurationService.SetSettingAsync("SecureDeletePasses", SecureDeletePasses);
        await _configurationService.SetSettingAsync("StartWithWindows", StartWithWindows);
        await _configurationService.SetSettingAsync("MinimizeToTray", MinimizeToTray);
        await _configurationService.SetSettingAsync("ShowNotifications", ShowNotifications);
        
        await _configurationService.SaveConfigurationAsync();
    }
}
