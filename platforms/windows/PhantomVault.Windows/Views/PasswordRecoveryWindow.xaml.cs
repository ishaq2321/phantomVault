using System.Windows;

namespace PhantomVault.Windows.Views;

public partial class PasswordRecoveryWindow : Window
{
    public PasswordRecoveryWindow()
    {
        InitializeComponent();
    }

    private void RecoverPassword_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = true;
        Close();
    }
}
