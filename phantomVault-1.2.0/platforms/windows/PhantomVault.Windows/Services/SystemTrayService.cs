using H.NotifyIcon;
using H.NotifyIcon.Core;
using System;
using System.Collections.Generic;
using System.Windows;

namespace PhantomVault.Windows.Services;

public class SystemTrayService : ISystemTrayService, IDisposable
{
    private TaskbarIcon? _taskbarIcon;
    private readonly List<Action> _menuItems = new();
    private bool _disposed = false;

    public bool IsVisible => _taskbarIcon?.IsVisible ?? false;

    public void Initialize()
    {
        if (_disposed) return;

        _taskbarIcon = new TaskbarIcon
        {
            Icon = new System.Drawing.Icon("Resources/icon.ico"),
            ToolTipText = "Phantom Vault - Secure Folder Management",
            Visibility = Visibility.Visible
        };

        _taskbarIcon.TrayMouseDoubleClick += OnTrayMouseDoubleClick;
    }

    public void SetIcon(string iconPath)
    {
        if (_disposed || _taskbarIcon == null) return;

        try
        {
            _taskbarIcon.Icon = new System.Drawing.Icon(iconPath);
        }
        catch (Exception)
        {
            // Use default icon if specified icon fails to load
        }
    }

    public void SetTooltip(string tooltip)
    {
        if (_disposed || _taskbarIcon == null) return;

        _taskbarIcon.ToolTipText = tooltip;
    }

    public void AddMenuItem(string text, Action callback)
    {
        if (_disposed) return;

        _menuItems.Add(callback);
        UpdateContextMenu();
    }

    public void AddSeparator()
    {
        if (_disposed) return;

        // Add separator to context menu
        UpdateContextMenu();
    }

    public void ClearMenu()
    {
        if (_disposed) return;

        _menuItems.Clear();
        UpdateContextMenu();
    }

    public void ShowNotification(string title, string message, NotificationType type = NotificationType.Info)
    {
        if (_disposed || _taskbarIcon == null) return;

        var icon = type switch
        {
            NotificationType.Success => NotificationIcon.Info,
            NotificationType.Warning => NotificationIcon.Warning,
            NotificationType.Error => NotificationIcon.Error,
            _ => NotificationIcon.Info
        };

        _taskbarIcon.ShowNotification(title, message, icon);
    }

    public void Hide()
    {
        if (_disposed || _taskbarIcon == null) return;

        _taskbarIcon.Visibility = Visibility.Hidden;
    }

    public void Show()
    {
        if (_disposed || _taskbarIcon == null) return;

        _taskbarIcon.Visibility = Visibility.Visible;
    }

    private void OnTrayMouseDoubleClick(object sender, RoutedEventArgs e)
    {
        // Show main window on double click
        var mainWindow = Application.Current.MainWindow;
        if (mainWindow != null)
        {
            mainWindow.Show();
            mainWindow.Activate();
        }
    }

    private void UpdateContextMenu()
    {
        if (_disposed || _taskbarIcon == null) return;

        // Create context menu
        var contextMenu = new System.Windows.Controls.ContextMenu();

        foreach (var menuItem in _menuItems)
        {
            var item = new System.Windows.Controls.MenuItem
            {
                Header = "Menu Item", // This would be set based on the action
                Command = new RelayCommand(menuItem)
            };
            contextMenu.Items.Add(item);
        }

        _taskbarIcon.ContextMenu = contextMenu;
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            _taskbarIcon?.Dispose();
            _disposed = true;
        }
    }
}

// Simple relay command for menu items
public class RelayCommand : System.Windows.Input.ICommand
{
    private readonly Action _execute;
    private readonly Func<bool>? _canExecute;

    public RelayCommand(Action execute, Func<bool>? canExecute = null)
    {
        _execute = execute ?? throw new ArgumentNullException(nameof(execute));
        _canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged
    {
        add { CommandManager.RequerySuggested += value; }
        remove { CommandManager.RequerySuggested -= value; }
    }

    public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

    public void Execute(object? parameter) => _execute();
}
