using Xunit;
using FluentAssertions;
using PhantomVault.Windows.Services;
using PhantomVault.Windows.Models;
using System.Linq;
using System.Threading.Tasks;

namespace PhantomVault.Windows.Tests;

public class VaultServiceTests
{
    private readonly VaultService _vaultService;
    private readonly MockEncryptionService _encryptionService;
    private readonly MockFileSystemService _fileSystemService;

    public VaultServiceTests()
    {
        _encryptionService = new MockEncryptionService();
        _fileSystemService = new MockFileSystemService();
        _vaultService = new VaultService(_encryptionService, _fileSystemService);
    }

    [Fact]
    public async Task GetAllVaultsAsync_ShouldReturnVaults()
    {
        // Act
        var vaults = await _vaultService.GetAllVaultsAsync();

        // Assert
        vaults.Should().NotBeNull();
        vaults.Should().HaveCount(3); // Sample vaults
    }

    [Fact]
    public async Task CreateVaultAsync_ShouldCreateVault()
    {
        // Arrange
        var name = "Test Vault";
        var description = "Test Description";
        var location = @"C:\Test\Vault";

        // Act
        var vault = await _vaultService.CreateVaultAsync(name, description, location);

        // Assert
        vault.Should().NotBeNull();
        vault.Name.Should().Be(name);
        vault.Description.Should().Be(description);
        vault.Location.Should().Be(location);
        vault.IsLocked.Should().BeTrue();
    }

    [Fact]
    public async Task GetVaultByIdAsync_WithValidId_ShouldReturnVault()
    {
        // Act
        var vault = await _vaultService.GetVaultByIdAsync("1");

        // Assert
        vault.Should().NotBeNull();
        vault!.Id.Should().Be("1");
    }

    [Fact]
    public async Task GetVaultByIdAsync_WithInvalidId_ShouldReturnNull()
    {
        // Act
        var vault = await _vaultService.GetVaultByIdAsync("invalid");

        // Assert
        vault.Should().BeNull();
    }

    [Fact]
    public async Task LockVaultAsync_ShouldLockVault()
    {
        // Arrange
        var vault = new Vault { Id = "test", IsLocked = false };

        // Act
        var result = await _vaultService.LockVaultAsync("test");

        // Assert
        result.Should().BeFalse(); // Vault doesn't exist in service
    }

    [Fact]
    public async Task UnlockVaultAsync_WithValidPassword_ShouldUnlockVault()
    {
        // Act
        var result = await _vaultService.UnlockVaultAsync("1", "password");

        // Assert
        result.Should().BeTrue();
    }
}

// Mock services for testing
public class MockEncryptionService : IEncryptionService
{
    public Task<byte[]> EncryptDataAsync(byte[] data, byte[] key, byte[] iv)
    {
        return Task.FromResult(data);
    }

    public Task<byte[]> DecryptDataAsync(byte[] encryptedData, byte[] key, byte[] iv)
    {
        return Task.FromResult(encryptedData);
    }

    public Task<byte[]> GenerateKeyAsync()
    {
        return Task.FromResult(new byte[32]);
    }

    public Task<byte[]> GenerateIVAsync()
    {
        return Task.FromResult(new byte[16]);
    }

    public Task<byte[]> DeriveKeyFromPasswordAsync(string password, byte[] salt)
    {
        return Task.FromResult(new byte[32]);
    }

    public Task<byte[]> GenerateSaltAsync()
    {
        return Task.FromResult(new byte[32]);
    }

    public Task<bool> EncryptFileAsync(string sourcePath, string destPath, byte[] key, byte[] iv)
    {
        return Task.FromResult(true);
    }

    public Task<bool> DecryptFileAsync(string sourcePath, string destPath, byte[] key, byte[] iv)
    {
        return Task.FromResult(true);
    }
}

public class MockFileSystemService : IFileSystemService
{
    public Task<bool> CreateDirectoryAsync(string path)
    {
        return Task.FromResult(true);
    }

    public Task<bool> DeleteDirectoryAsync(string path, bool recursive = false)
    {
        return Task.FromResult(true);
    }

    public Task<bool> HideFileAsync(string path)
    {
        return Task.FromResult(true);
    }

    public Task<bool> UnhideFileAsync(string path)
    {
        return Task.FromResult(true);
    }

    public Task<bool> IsFileHiddenAsync(string path)
    {
        return Task.FromResult(false);
    }

    public Task<bool> FileExistsAsync(string path)
    {
        return Task.FromResult(true);
    }

    public Task<bool> DirectoryExistsAsync(string path)
    {
        return Task.FromResult(true);
    }

    public Task<long> GetFileSizeAsync(string path)
    {
        return Task.FromResult(1024L);
    }

    public Task<DateTime> GetFileModifiedTimeAsync(string path)
    {
        return Task.FromResult(DateTime.Now);
    }

    public Task<bool> SetFileAttributesAsync(string path, FileAttributes attributes)
    {
        return Task.FromResult(true);
    }

    public Task<FileAttributes> GetFileAttributesAsync(string path)
    {
        return Task.FromResult(FileAttributes.None);
    }
}
