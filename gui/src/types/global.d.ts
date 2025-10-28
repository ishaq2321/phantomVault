/**
 * Global TypeScript declarations for PhantomVault GUI
 */

interface PhantomVaultAPI {
    // App information
    app: {
        getVersion(): Promise<string>;
        isAdmin(): Promise<boolean>;
    };

    // Service management
    service: {
        getStatus(): Promise<{ running: boolean; pid: number | null }>;
        restart(): Promise<boolean>;
    };

    // Dialog operations
    dialog: {
        showMessage(options: Electron.MessageBoxOptions): Promise<Electron.MessageBoxReturnValue>;
        showOpenDialog(options: Electron.OpenDialogOptions): Promise<Electron.OpenDialogReturnValue>;
        showSaveDialog(options: Electron.SaveDialogOptions): Promise<Electron.SaveDialogReturnValue>;
    };

    // IPC communication with service
    ipc: {
        // Profile operations
        createProfile(name: string, masterKey: string): Promise<any>;
        getAllProfiles(): Promise<any>;
        authenticateProfile(profileId: string, masterKey: string): Promise<any>;
        changeProfilePassword(profileId: string, oldKey: string, newKey: string): Promise<any>;

        // Enhanced vault operations (AES-256 encryption)
        lockFolder(profileId: string, folderPath: string, masterKey: string): Promise<any>;
        unlockFoldersTemporary(profileId: string, masterKey: string): Promise<any>;
        unlockFoldersPermanent(profileId: string, masterKey: string, folderIds: string[]): Promise<any>;
        getVaultStats(profileId: string): Promise<any>;

        // Folder operations (legacy and new)
        addFolder(profileId: string, folderPath: string): Promise<any>;
        getProfileFolders(profileId: string): Promise<any>;
        unlockFolderTemporary(profileId: string, folderId: string): Promise<any>;
        unlockFolderPermanent(profileId: string, folderId: string): Promise<any>;
        lockTemporaryFolders(profileId: string): Promise<any>;

        // Analytics operations
        getProfileAnalytics(profileId: string, timeRange: string): Promise<any>;
        getSystemAnalytics(timeRange: string): Promise<any>;

        // Enhanced recovery operations (AES-256 with PBKDF2)
        recoverWithKey(recoveryKey: string): Promise<any>;
        generateRecoveryKey(profileId: string): Promise<any>;
        getCurrentRecoveryKey(profileId: string, masterKey: string): Promise<any>;
        changePassword(profileId: string, currentPassword: string, newPassword: string): Promise<any>;

        // Platform operations
        getPlatformInfo(): Promise<any>;
        getUnlockMethods(): Promise<any>;
        setPreferredUnlockMethod(method: string): Promise<any>;
    };

    // Event listeners
    on: {
        serviceStatusChanged(callback: (status: any) => void): void;
        folderStatusChanged(callback: (status: any) => void): void;
        securityEvent(callback: (event: any) => void): void;
    };

    // Remove event listeners
    off: {
        serviceStatusChanged(callback: (status: any) => void): void;
        folderStatusChanged(callback: (status: any) => void): void;
        securityEvent(callback: (event: any) => void): void;
    };
}

declare global {
    interface Window {
        phantomVault: PhantomVaultAPI;
    }
}

export { };