/**
 * Simple KeyboardSequenceManager for UI compatibility
 */

// Type stubs for compatibility
const SequenceValidationResult = {};
const SequenceConflict = {};

class KeyboardSequenceManager {
    constructor() {
        this.sequences = new Map();
        this.isInitialized = false;
    }

    async initialize() {
        this.isInitialized = true;
        console.log('[KeyboardSequenceManager] Initialized (UI stub)');
        return true;
    }

    async validateSequence(sequence) {
        return {
            isValid: sequence && sequence.length >= 2,
            errors: sequence && sequence.length >= 2 ? [] : ['Sequence too short']
        };
    }

    async setVaultSequence(vaultId, sequence) {
        this.sequences.set(vaultId, sequence);
        return { success: true };
    }

    async getVaultSequence(vaultId) {
        return this.sequences.get(vaultId) || '';
    }
}

// Singleton instance
let globalSequenceManager = null;

function getKeyboardSequenceManager() {
    if (!globalSequenceManager) {
        globalSequenceManager = new KeyboardSequenceManager();
    }
    return globalSequenceManager;
}

async function initializeKeyboardSequenceManager() {
    const manager = getKeyboardSequenceManager();
    await manager.initialize();
    return manager;
}

module.exports = {
    KeyboardSequenceManager,
    getKeyboardSequenceManager,
    initializeKeyboardSequenceManager,
    SequenceValidationResult,
    SequenceConflict
};