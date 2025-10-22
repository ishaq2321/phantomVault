using System;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public class NotificationService : INotificationService
{
    private readonly ISystemTrayService _systemTrayService;

    public NotificationService(ISystemTrayService systemTrayService)
    {
        _systemTrayService = systemTrayService;
    }

    public void ShowInfo(string message)
    {
        ShowNotification("Phantom Vault", message, NotificationType.Info);
    }

    public void ShowWarning(string message)
    {
        ShowNotification("Phantom Vault - Warning", message, NotificationType.Warning);
    }

    public void ShowError(string message)
    {
        ShowNotification("Phantom Vault - Error", message, NotificationType.Error);
    }

    public void ShowSuccess(string message)
    {
        ShowNotification("Phantom Vault - Success", message, NotificationType.Success);
    }

    public void ShowNotification(string title, string message, NotificationType type = NotificationType.Info)
    {
        try
        {
            _systemTrayService.ShowNotification(title, message, type);
        }
        catch (Exception)
        {
            // Fallback to console if system tray notification fails
            Console.WriteLine($"[{type}] {title}: {message}");
        }
    }
}
