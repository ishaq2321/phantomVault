namespace PhantomVault.Windows.Services;

public interface INotificationService
{
    void ShowInfo(string message);
    void ShowWarning(string message);
    void ShowError(string message);
    void ShowSuccess(string message);
    void ShowNotification(string title, string message, NotificationType type = NotificationType.Info);
}
