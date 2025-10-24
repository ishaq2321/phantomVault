/**
 * Simple VaultProfileManager for UI compatibility
 */

class VaultProfileManager {
    constructor() {
        this.profiles = new Map();
        this.currentProfile = null;
    }

    static getInstance() {
        if (!VaultProfileManager.instance) {
            VaultProfileManager.instance = new VaultProfileManager();
        }
        return VaultProfileManager.instance;
    }

    async initialize() {
        console.log('[VaultProfileManager] Initialized (UI stub)');
        return true;
    }

    async createProfile(profileData) {
        console.log('[VaultProfileManager] Creating profile:', profileData.name);
        return { success: true, profileId: Date.now().toString() };
    }

    async getProfiles() {
        return Array.from(this.profiles.values());
    }

    async getCurrentProfile() {
        return this.currentProfile;
    }
}

module.exports = { VaultProfileManager };