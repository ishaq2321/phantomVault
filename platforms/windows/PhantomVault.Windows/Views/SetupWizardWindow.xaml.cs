using System.Windows;

namespace PhantomVault.Windows.Views;

public partial class SetupWizardWindow : Window
{
    public SetupWizardWindow()
    {
        InitializeComponent();
    }

    private void GetStarted_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = true;
        Close();
    }
}
