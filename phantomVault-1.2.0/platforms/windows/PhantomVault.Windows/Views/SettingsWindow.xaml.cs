using System.Windows;

namespace PhantomVault.Windows.Views;

public partial class SettingsWindow : Window
{
    public SettingsWindow()
    {
        InitializeComponent();
    }

    private void SaveSettings_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = true;
        Close();
    }
}
