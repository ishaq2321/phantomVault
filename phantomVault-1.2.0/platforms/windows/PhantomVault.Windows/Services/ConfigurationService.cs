using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public class ConfigurationService : IConfigurationService
{
    private readonly Dictionary<string, object> _settings = new();
    private readonly string _configPath;
    private readonly JsonSerializerOptions _jsonOptions;

    public ConfigurationService()
    {
        var appDataPath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        var configDir = Path.Combine(appDataPath, "PhantomVault");
        Directory.CreateDirectory(configDir);
        _configPath = Path.Combine(configDir, "config.json");
        
        _jsonOptions = new JsonSerializerOptions
        {
            WriteIndented = true,
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase
        };
    }

    public async Task<T> GetSettingAsync<T>(string key, T defaultValue = default)
    {
        if (_settings.TryGetValue(key, out var value))
        {
            if (value is JsonElement jsonElement)
            {
                try
                {
                    return JsonSerializer.Deserialize<T>(jsonElement.GetRawText(), _jsonOptions);
                }
                catch
                {
                    return defaultValue;
                }
            }
            
            if (value is T directValue)
            {
                return directValue;
            }
        }
        
        return defaultValue;
    }

    public async Task SetSettingAsync<T>(string key, T value)
    {
        _settings[key] = value;
        await SaveConfigurationAsync();
    }

    public Task<bool> HasSettingAsync(string key)
    {
        return Task.FromResult(_settings.ContainsKey(key));
    }

    public async Task RemoveSettingAsync(string key)
    {
        _settings.Remove(key);
        await SaveConfigurationAsync();
    }

    public async Task ClearAllSettingsAsync()
    {
        _settings.Clear();
        await SaveConfigurationAsync();
    }

    public async Task SaveConfigurationAsync()
    {
        try
        {
            var json = JsonSerializer.Serialize(_settings, _jsonOptions);
            await File.WriteAllTextAsync(_configPath, json);
        }
        catch (Exception ex)
        {
            // Log error but don't throw to avoid breaking the application
            Console.WriteLine($"Failed to save configuration: {ex.Message}");
        }
    }

    public async Task LoadConfigurationAsync()
    {
        try
        {
            if (File.Exists(_configPath))
            {
                var json = await File.ReadAllTextAsync(_configPath);
                var settings = JsonSerializer.Deserialize<Dictionary<string, object>>(json, _jsonOptions);
                
                if (settings != null)
                {
                    _settings.Clear();
                    foreach (var kvp in settings)
                    {
                        _settings[kvp.Key] = kvp.Value;
                    }
                }
            }
        }
        catch (Exception ex)
        {
            // Log error but don't throw to avoid breaking the application
            Console.WriteLine($"Failed to load configuration: {ex.Message}");
        }
    }
}
