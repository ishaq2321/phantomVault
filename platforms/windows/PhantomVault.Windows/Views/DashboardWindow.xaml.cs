using PhantomVault.Windows.ViewModels;

namespace PhantomVault.Windows.Views;

public partial class DashboardWindow : Window
{
    public DashboardWindow(DashboardViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
    }
}
