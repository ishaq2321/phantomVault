using System;

namespace PhantomVault.Windows.Services;

public interface ISystemTrayService
{
    void Initialize();
    void SetIcon(string iconPath);
    void SetTooltip(string tooltip);
    void AddMenuItem(string text, Action callback);
    void AddSeparator();
    void ClearMenu();
    void ShowNotification(string title, string message, NotificationType type = NotificationType.Info);
    void Hide();
    void Show();
    bool IsVisible { get; }
}

public enum NotificationType
{
    Info,
    Warning,
    Error,
    Success
}
