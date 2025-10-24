/**
 * Simple VaultFolderManager for UI compatibility
 */

class VaultFolderManager {
    constructor() {
        this.vaults = new Map();
    }

    static getInstance() {
        if (!VaultFolderManager.instance) {
            VaultFolderManager.instance = new VaultFolderManager();
        }
        return VaultFolderManager.instance;
    }

    async initialize() {
        console.log('[VaultFolderManager] Initialized (UI stub)');
        return true;
    }

    async createVault(vaultData) {
        console.log('[VaultFolderManager] Creating vault:', vaultData.name);
        return { success: true, vaultId: Date.now().toString() };
    }

    async getVaults() {
        return Array.from(this.vaults.values());
    }

    async getVaultStatus(vaultId) {
        return { status: 'locked', isVisible: false };
    }
}

module.exports = { VaultFolderManager };