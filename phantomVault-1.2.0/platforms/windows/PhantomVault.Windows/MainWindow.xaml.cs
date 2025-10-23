using System.Windows;
using PhantomVault.Windows.ViewModels;

namespace PhantomVault.Windows;

public partial class MainWindow : Window
{
    public MainWindow(MainWindowViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
        
        // Handle window state changes
        StateChanged += OnStateChanged;
        Closing += OnClosing;
    }

    private void OnStateChanged(object? sender, EventArgs e)
    {
        if (WindowState == WindowState.Minimized)
        {
            // Hide window and show in system tray
            Hide();
        }
    }

    private void OnClosing(object? sender, System.ComponentModel.CancelEventArgs e)
    {
        // If user is closing window, minimize to tray instead
        if (DataContext is MainWindowViewModel viewModel && viewModel.KeepInTray)
        {
            e.Cancel = true;
            WindowState = WindowState.Minimized;
            Hide();
        }
    }

    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);
        
        // Set up global keyboard hooks
        if (DataContext is MainWindowViewModel viewModel)
        {
            viewModel.InitializeKeyboardHooks();
        }
    }
}
