using CommunityToolkit.Mvvm.ComponentModel;

namespace PhantomVault.Windows.ViewModels;

public partial class SetupWizardViewModel : ObservableObject
{
    [ObservableProperty]
    private string _vaultName = string.Empty;

    [ObservableProperty]
    private string _vaultDescription = string.Empty;

    [ObservableProperty]
    private string _vaultLocation = string.Empty;

    [ObservableProperty]
    private string _masterPassword = string.Empty;

    [ObservableProperty]
    private string _confirmPassword = string.Empty;

    [ObservableProperty]
    private bool _enablePasswordRecovery = true;

    [ObservableProperty]
    private int _currentStep = 1;

    [ObservableProperty]
    private int _totalSteps = 4;
}
