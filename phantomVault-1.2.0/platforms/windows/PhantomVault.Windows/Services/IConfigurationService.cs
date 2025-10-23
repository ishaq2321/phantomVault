using System;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public interface IConfigurationService
{
    Task<T> GetSettingAsync<T>(string key, T defaultValue = default);
    Task SetSettingAsync<T>(string key, T value);
    Task<bool> HasSettingAsync(string key);
    Task RemoveSettingAsync(string key);
    Task ClearAllSettingsAsync();
    Task SaveConfigurationAsync();
    Task LoadConfigurationAsync();
}
