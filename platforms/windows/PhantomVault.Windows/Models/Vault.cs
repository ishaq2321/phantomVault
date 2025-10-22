using System;

namespace PhantomVault.Windows.Models;

public class Vault
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Location { get; set; } = string.Empty;
    public DateTime CreatedTime { get; set; }
    public DateTime ModifiedTime { get; set; }
    public bool IsLocked { get; set; } = true;
    public bool IsSecure { get; set; } = true;
    public long Size { get; set; }
    public string Status { get; set; } = "Locked";
}

public class RecoveryQuestion
{
    public string Id { get; set; } = string.Empty;
    public string Question { get; set; } = string.Empty;
    public string Answer { get; set; } = string.Empty;
}

public class VaultConfiguration
{
    public bool AutoLock { get; set; } = true;
    public TimeSpan LockTimeout { get; set; } = TimeSpan.FromMinutes(30);
    public bool ClearClipboard { get; set; } = true;
    public TimeSpan ClipboardTimeout { get; set; } = TimeSpan.FromMinutes(5);
    public bool HideVaultDirectory { get; set; } = true;
    public bool SecureDelete { get; set; } = false;
    public int SecureDeletePasses { get; set; } = 3;
}
