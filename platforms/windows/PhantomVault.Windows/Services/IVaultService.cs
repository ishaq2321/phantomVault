using PhantomVault.Windows.Models;

namespace PhantomVault.Windows.Services;

public interface IVaultService
{
    Task<IEnumerable<Vault>> GetAllVaultsAsync();
    Task<Vault?> GetVaultByIdAsync(string id);
    Task<Vault> CreateVaultAsync(string name, string description, string location);
    Task<bool> UpdateVaultAsync(Vault vault);
    Task<bool> DeleteVaultAsync(string id);
    Task<bool> UnlockVaultAsync(string id, string password);
    Task<bool> LockVaultAsync(string id);
    Task<bool> IsVaultLockedAsync(string id);
    Task<bool> SetupPasswordRecoveryAsync(string vaultId, IEnumerable<RecoveryQuestion> questions);
    Task<bool> VerifyRecoveryAnswersAsync(string vaultId, IEnumerable<string> answers);
    Task<IEnumerable<RecoveryQuestion>> GetRecoveryQuestionsAsync(string vaultId);
}
