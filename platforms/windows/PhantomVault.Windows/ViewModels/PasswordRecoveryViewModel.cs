using CommunityToolkit.Mvvm.ComponentModel;
using PhantomVault.Windows.Models;

namespace PhantomVault.Windows.ViewModels;

public partial class PasswordRecoveryViewModel : ObservableObject
{
    [ObservableProperty]
    private string _vaultId = string.Empty;

    [ObservableProperty]
    private ObservableCollection<RecoveryQuestion> _questions = new();

    [ObservableProperty]
    private ObservableCollection<string> _answers = new();

    [ObservableProperty]
    private int _currentQuestionIndex = 0;

    [ObservableProperty]
    private bool _isVerifying = false;

    [ObservableProperty]
    private string _statusMessage = string.Empty;
}
