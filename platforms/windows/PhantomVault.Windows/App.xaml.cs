using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using PhantomVault.Windows.Services;
using PhantomVault.Windows.ViewModels;
using PhantomVault.Windows.Views;
using System.Windows;

namespace PhantomVault.Windows;

public partial class App : Application
{
    private IHost? _host;

    protected override async void OnStartup(StartupEventArgs e)
    {
        // Configure services
        _host = Host.CreateDefaultBuilder()
            .ConfigureServices((context, services) =>
            {
                // Register services
                services.AddSingleton<IVaultService, VaultService>();
                services.AddSingleton<IEncryptionService, EncryptionService>();
                services.AddSingleton<IFileSystemService, FileSystemService>();
                services.AddSingleton<ISystemTrayService, SystemTrayService>();
                services.AddSingleton<IKeyboardHookService, KeyboardHookService>();
                services.AddSingleton<INotificationService, NotificationService>();
                services.AddSingleton<IConfigurationService, ConfigurationService>();

                // Register ViewModels
                services.AddTransient<MainWindowViewModel>();
                services.AddTransient<SetupWizardViewModel>();
                services.AddTransient<DashboardViewModel>();
                services.AddTransient<PasswordRecoveryViewModel>();
                services.AddTransient<SettingsViewModel>();

                // Register Views
                services.AddTransient<MainWindow>();
                services.AddTransient<SetupWizardWindow>();
                services.AddTransient<DashboardWindow>();
                services.AddTransient<PasswordRecoveryWindow>();
                services.AddTransient<SettingsWindow>();
            })
            .ConfigureLogging(logging =>
            {
                logging.AddConsole();
                logging.SetMinimumLevel(LogLevel.Information);
            })
            .Build();

        await _host.StartAsync();

        // Get main window from DI container
        var mainWindow = _host.Services.GetRequiredService<MainWindow>();
        mainWindow.Show();

        base.OnStartup(e);
    }

    protected override async void OnExit(ExitEventArgs e)
    {
        if (_host != null)
        {
            await _host.StopAsync();
            _host.Dispose();
        }
        base.OnExit(e);
    }
}
