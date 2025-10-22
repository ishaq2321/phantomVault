using System;
using System.IO;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Services;

public class EncryptionService : IEncryptionService
{
    private const int KeySize = 256;
    private const int IVSize = 128;
    private const int SaltSize = 32;
    private const int Iterations = 100000;

    public async Task<byte[]> EncryptDataAsync(byte[] data, byte[] key, byte[] iv)
    {
        return await Task.Run(() =>
        {
            using var aes = Aes.Create();
            aes.KeySize = KeySize;
            aes.BlockSize = 128;
            aes.Mode = CipherMode.CBC;
            aes.Padding = PaddingMode.PKCS7;
            aes.Key = key;
            aes.IV = iv;

            using var encryptor = aes.CreateEncryptor();
            using var msEncrypt = new MemoryStream();
            using var csEncrypt = new CryptoStream(msEncrypt, encryptor, CryptoStreamMode.Write);
            using var swEncrypt = new StreamWriter(csEncrypt);
            
            swEncrypt.Write(Convert.ToBase64String(data));
            swEncrypt.Close();
            
            return msEncrypt.ToArray();
        });
    }

    public async Task<byte[]> DecryptDataAsync(byte[] encryptedData, byte[] key, byte[] iv)
    {
        return await Task.Run(() =>
        {
            using var aes = Aes.Create();
            aes.KeySize = KeySize;
            aes.BlockSize = 128;
            aes.Mode = CipherMode.CBC;
            aes.Padding = PaddingMode.PKCS7;
            aes.Key = key;
            aes.IV = iv;

            using var decryptor = aes.CreateDecryptor();
            using var msDecrypt = new MemoryStream(encryptedData);
            using var csDecrypt = new CryptoStream(msDecrypt, decryptor, CryptoStreamMode.Read);
            using var srDecrypt = new StreamReader(csDecrypt);
            
            var decryptedString = srDecrypt.ReadToEnd();
            return Convert.FromBase64String(decryptedString);
        });
    }

    public async Task<byte[]> GenerateKeyAsync()
    {
        return await Task.Run(() =>
        {
            using var aes = Aes.Create();
            aes.KeySize = KeySize;
            aes.GenerateKey();
            return aes.Key;
        });
    }

    public async Task<byte[]> GenerateIVAsync()
    {
        return await Task.Run(() =>
        {
            using var aes = Aes.Create();
            aes.BlockSize = 128;
            aes.GenerateIV();
            return aes.IV;
        });
    }

    public async Task<byte[]> DeriveKeyFromPasswordAsync(string password, byte[] salt)
    {
        return await Task.Run(() =>
        {
            using var pbkdf2 = new Rfc2898DeriveBytes(password, salt, Iterations, HashAlgorithmName.SHA256);
            return pbkdf2.GetBytes(KeySize / 8);
        });
    }

    public async Task<byte[]> GenerateSaltAsync()
    {
        return await Task.Run(() =>
        {
            var salt = new byte[SaltSize];
            using var rng = RandomNumberGenerator.Create();
            rng.GetBytes(salt);
            return salt;
        });
    }

    public async Task<bool> EncryptFileAsync(string sourcePath, string destPath, byte[] key, byte[] iv)
    {
        try
        {
            var data = await File.ReadAllBytesAsync(sourcePath);
            var encryptedData = await EncryptDataAsync(data, key, iv);
            await File.WriteAllBytesAsync(destPath, encryptedData);
            return true;
        }
        catch
        {
            return false;
        }
    }

    public async Task<bool> DecryptFileAsync(string sourcePath, string destPath, byte[] key, byte[] iv)
    {
        try
        {
            var encryptedData = await File.ReadAllBytesAsync(sourcePath);
            var decryptedData = await DecryptDataAsync(encryptedData, key, iv);
            await File.WriteAllBytesAsync(destPath, decryptedData);
            return true;
        }
        catch
        {
            return false;
        }
    }
}
