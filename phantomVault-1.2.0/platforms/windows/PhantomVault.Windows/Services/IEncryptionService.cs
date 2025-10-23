using System;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public interface IEncryptionService
{
    Task<byte[]> EncryptDataAsync(byte[] data, byte[] key, byte[] iv);
    Task<byte[]> DecryptDataAsync(byte[] encryptedData, byte[] key, byte[] iv);
    Task<byte[]> GenerateKeyAsync();
    Task<byte[]> GenerateIVAsync();
    Task<byte[]> DeriveKeyFromPasswordAsync(string password, byte[] salt);
    Task<byte[]> GenerateSaltAsync();
    Task<bool> EncryptFileAsync(string sourcePath, string destPath, byte[] key, byte[] iv);
    Task<bool> DecryptFileAsync(string sourcePath, string destPath, byte[] key, byte[] iv);
}
