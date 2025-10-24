"use strict";
/**
 * Keyboard Sequence Manager Service
 *
 * Manages keyboard sequences for vaults including validation, conflict detection,
 * and integration with the sequence detector system
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.KeyboardSequenceManager = void 0;
exports.getKeyboardSequenceManager = getKeyboardSequenceManager;
exports.initializeKeyboardSequenceManager = initializeKeyboardSequenceManager;
/**
 * Keyboard sequence management service
 */
class KeyboardSequenceManager {
    constructor() {
        this.sequences = new Map();
        this.sequencesByVault = new Map();
        this.isInitialized = false;
    }
    // ==================== INITIALIZATION ====================
    /**
     * Initialize the sequence manager
     */
    async initialize() {
        if (this.isInitialized) {
            return;
        }
        try {
            await this.loadSequences();
            this.isInitialized = true;
            console.log('KeyboardSequenceManager initialized');
        }
        catch (error) {
            console.error('Failed to initialize KeyboardSequenceManager:', error);
            throw error;
        }
    }
    /**
     * Load sequences from storage
     */
    async loadSequences() {
        try {
            // In a real implementation, this would load from the backend
            // For now, we'll simulate with some example sequences
            const mockSequences = [
                {
                    id: 'seq-1',
                    vaultId: 'vault-1',
                    vaultName: 'Documents',
                    sequence: 'T1234',
                    mode: 'temporary',
                    isActive: true,
                    createdAt: new Date(Date.now() - 86400000), // 1 day ago
                    lastUsed: new Date(Date.now() - 3600000), // 1 hour ago
                    useCount: 15,
                },
                {
                    id: 'seq-2',
                    vaultId: 'vault-2',
                    vaultName: 'Photos',
                    sequence: 'Pphotos123',
                    mode: 'permanent',
                    isActive: true,
                    createdAt: new Date(Date.now() - 172800000), // 2 days ago
                    lastUsed: new Date(Date.now() - 7200000), // 2 hours ago
                    useCount: 8,
                },
            ];
            // Clear existing data
            this.sequences.clear();
            this.sequencesByVault.clear();
            // Load sequences
            for (const sequence of mockSequences) {
                this.sequences.set(sequence.id, sequence);
                if (!this.sequencesByVault.has(sequence.vaultId)) {
                    this.sequencesByVault.set(sequence.vaultId, []);
                }
                this.sequencesByVault.get(sequence.vaultId).push(sequence.id);
            }
            console.log(`Loaded ${mockSequences.length} keyboard sequences`);
        }
        catch (error) {
            console.error('Failed to load keyboard sequences:', error);
            throw error;
        }
    }
    // ==================== SEQUENCE MANAGEMENT ====================
    /**
     * Get all sequences
     */
    getAllSequences() {
        return Array.from(this.sequences.values());
    }
    /**
     * Get sequences for a specific vault
     */
    getSequencesForVault(vaultId) {
        const sequenceIds = this.sequencesByVault.get(vaultId) || [];
        return sequenceIds
            .map(id => this.sequences.get(id))
            .filter((seq) => seq !== undefined);
    }
    /**
     * Get sequence by ID
     */
    getSequence(sequenceId) {
        return this.sequences.get(sequenceId);
    }
    /**
     * Add or update a sequence
     */
    async setSequence(vaultId, vaultName, sequence, mode = 'auto') {
        // Validate the sequence
        const validation = this.validateSequence(sequence, vaultId);
        if (!validation.isValid) {
            throw new Error(`Invalid sequence: ${validation.errors.join(', ')}`);
        }
        // Check for conflicts
        const conflicts = this.findConflicts(sequence, vaultId);
        if (conflicts.length > 0) {
            const exactConflict = conflicts.find(c => c.conflictType === 'exact');
            if (exactConflict) {
                throw new Error(`Sequence "${sequence}" is already in use by vault "${exactConflict.existingSequence.vaultName}"`);
            }
        }
        // Remove existing sequences for this vault
        await this.removeSequencesForVault(vaultId);
        // Create new sequence
        const newSequence = {
            id: `seq-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
            vaultId,
            vaultName,
            sequence,
            mode,
            isActive: true,
            createdAt: new Date(),
            useCount: 0,
        };
        // Store the sequence
        this.sequences.set(newSequence.id, newSequence);
        if (!this.sequencesByVault.has(vaultId)) {
            this.sequencesByVault.set(vaultId, []);
        }
        this.sequencesByVault.get(vaultId).push(newSequence.id);
        // Save to backend (simulated)
        await this.saveSequences();
        console.log(`Set keyboard sequence for vault "${vaultName}": ${sequence}`);
        return newSequence;
    }
    /**
     * Remove sequence by ID
     */
    async removeSequence(sequenceId) {
        const sequence = this.sequences.get(sequenceId);
        if (!sequence) {
            return false;
        }
        // Remove from sequences map
        this.sequences.delete(sequenceId);
        // Remove from vault mapping
        const vaultSequences = this.sequencesByVault.get(sequence.vaultId);
        if (vaultSequences) {
            const index = vaultSequences.indexOf(sequenceId);
            if (index > -1) {
                vaultSequences.splice(index, 1);
            }
            // Clean up empty vault entries
            if (vaultSequences.length === 0) {
                this.sequencesByVault.delete(sequence.vaultId);
            }
        }
        // Save to backend (simulated)
        await this.saveSequences();
        console.log(`Removed keyboard sequence: ${sequence.sequence}`);
        return true;
    }
    /**
     * Remove all sequences for a vault
     */
    async removeSequencesForVault(vaultId) {
        const sequenceIds = this.sequencesByVault.get(vaultId) || [];
        let removedCount = 0;
        for (const sequenceId of sequenceIds) {
            if (await this.removeSequence(sequenceId)) {
                removedCount++;
            }
        }
        return removedCount;
    }
    // ==================== VALIDATION ====================
    /**
     * Validate a keyboard sequence
     */
    validateSequence(sequence, excludeVaultId) {
        const errors = [];
        const warnings = [];
        const suggestions = [];
        // Basic validation
        if (!sequence || sequence.trim().length === 0) {
            errors.push('Sequence cannot be empty');
            return { isValid: false, errors, warnings, suggestions };
        }
        // Length validation
        if (sequence.length < 2) {
            errors.push('Sequence must be at least 2 characters long');
        }
        if (sequence.length > 20) {
            errors.push('Sequence must be 20 characters or less');
        }
        // Character validation
        if (!/^[a-zA-Z0-9!@#$%^&*()_+\-=\[\]{}|;:,.<>?]+$/.test(sequence)) {
            errors.push('Sequence contains invalid characters');
            suggestions.push('Use only letters, numbers, and basic symbols');
        }
        // Pattern analysis
        const patterns = this.analyzeSequencePatterns(sequence);
        if (patterns.isCommonPattern) {
            warnings.push('Sequence uses a common pattern');
            suggestions.push('Consider using a more unique sequence for better security');
        }
        if (patterns.hasRepeatingChars && patterns.repeatingRatio > 0.5) {
            warnings.push('Sequence has many repeating characters');
            suggestions.push('Mix different characters for better uniqueness');
        }
        if (patterns.isSequential) {
            warnings.push('Sequence uses sequential characters (e.g., 1234, abcd)');
            suggestions.push('Avoid sequential patterns for better security');
        }
        // Conflict checking
        const conflicts = this.findConflicts(sequence, excludeVaultId);
        for (const conflict of conflicts) {
            if (conflict.conflictType === 'exact') {
                errors.push(`Sequence is already in use by vault "${conflict.existingSequence.vaultName}"`);
            }
            else if (conflict.conflictType === 'similar' && conflict.similarity > 0.8) {
                warnings.push(`Sequence is very similar to existing sequence for vault "${conflict.existingSequence.vaultName}"`);
                suggestions.push('Consider using a more distinct sequence');
            }
        }
        // Mode validation
        const firstChar = sequence[0].toLowerCase();
        if (firstChar === 't' || firstChar === 'p') {
            if (sequence.length < 3) {
                errors.push('Mode prefix (T/P) requires at least 2 additional characters');
            }
        }
        return {
            isValid: errors.length === 0,
            errors,
            warnings,
            suggestions,
        };
    }
    /**
     * Find conflicts with existing sequences
     */
    findConflicts(sequence, excludeVaultId) {
        const conflicts = [];
        const normalizedSequence = sequence.toLowerCase();
        for (const existingSequence of this.sequences.values()) {
            // Skip sequences from the same vault if excluding
            if (excludeVaultId && existingSequence.vaultId === excludeVaultId) {
                continue;
            }
            const existingNormalized = existingSequence.sequence.toLowerCase();
            // Exact match
            if (normalizedSequence === existingNormalized) {
                conflicts.push({
                    existingSequence,
                    conflictType: 'exact',
                    similarity: 1.0,
                });
                continue;
            }
            // Similarity check
            const similarity = this.calculateSimilarity(normalizedSequence, existingNormalized);
            if (similarity > 0.7) {
                conflicts.push({
                    existingSequence,
                    conflictType: 'similar',
                    similarity,
                });
            }
            // Prefix check
            if (normalizedSequence.startsWith(existingNormalized) || existingNormalized.startsWith(normalizedSequence)) {
                conflicts.push({
                    existingSequence,
                    conflictType: 'prefix',
                    similarity: Math.min(normalizedSequence.length, existingNormalized.length) /
                        Math.max(normalizedSequence.length, existingNormalized.length),
                });
            }
        }
        return conflicts;
    }
    // ==================== PATTERN ANALYSIS ====================
    /**
     * Analyze sequence patterns
     */
    analyzeSequencePatterns(sequence) {
        const lower = sequence.toLowerCase();
        // Common patterns
        const commonPatterns = [
            '1234', '4321', 'abcd', 'dcba', 'qwerty', 'asdf', 'zxcv',
            'password', 'admin', 'user', 'test', 'demo', 'vault'
        ];
        const isCommonPattern = commonPatterns.some(pattern => lower.includes(pattern));
        // Repeating characters
        const charCounts = new Map();
        for (const char of lower) {
            charCounts.set(char, (charCounts.get(char) || 0) + 1);
        }
        const maxRepeats = Math.max(...charCounts.values());
        const hasRepeatingChars = maxRepeats > 1;
        const repeatingRatio = maxRepeats / sequence.length;
        // Sequential patterns
        let isSequential = false;
        if (sequence.length >= 3) {
            for (let i = 0; i < sequence.length - 2; i++) {
                const char1 = sequence.charCodeAt(i);
                const char2 = sequence.charCodeAt(i + 1);
                const char3 = sequence.charCodeAt(i + 2);
                if ((char2 === char1 + 1 && char3 === char2 + 1) ||
                    (char2 === char1 - 1 && char3 === char2 - 1)) {
                    isSequential = true;
                    break;
                }
            }
        }
        return {
            isCommonPattern,
            hasRepeatingChars,
            repeatingRatio,
            isSequential,
        };
    }
    /**
     * Calculate similarity between two sequences
     */
    calculateSimilarity(seq1, seq2) {
        if (seq1 === seq2)
            return 1.0;
        if (seq1.length === 0 || seq2.length === 0)
            return 0.0;
        // Levenshtein distance
        const matrix = [];
        for (let i = 0; i <= seq2.length; i++) {
            matrix[i] = [i];
        }
        for (let j = 0; j <= seq1.length; j++) {
            matrix[0][j] = j;
        }
        for (let i = 1; i <= seq2.length; i++) {
            for (let j = 1; j <= seq1.length; j++) {
                if (seq2.charAt(i - 1) === seq1.charAt(j - 1)) {
                    matrix[i][j] = matrix[i - 1][j - 1];
                }
                else {
                    matrix[i][j] = Math.min(matrix[i - 1][j - 1] + 1, // substitution
                    matrix[i][j - 1] + 1, // insertion
                    matrix[i - 1][j] + 1 // deletion
                    );
                }
            }
        }
        const distance = matrix[seq2.length][seq1.length];
        const maxLength = Math.max(seq1.length, seq2.length);
        return 1 - (distance / maxLength);
    }
    // ==================== USAGE TRACKING ====================
    /**
     * Record sequence usage
     */
    async recordUsage(sequenceId) {
        const sequence = this.sequences.get(sequenceId);
        if (!sequence) {
            return;
        }
        sequence.lastUsed = new Date();
        sequence.useCount++;
        // Save to backend (simulated)
        await this.saveSequences();
        console.log(`Recorded usage for sequence: ${sequence.sequence} (count: ${sequence.useCount})`);
    }
    /**
     * Get usage statistics
     */
    getUsageStats() {
        const allSequences = this.getAllSequences();
        const activeSequences = allSequences.filter(seq => seq.isActive);
        const totalUsage = allSequences.reduce((sum, seq) => sum + seq.useCount, 0);
        const mostUsedSequence = allSequences.reduce((max, seq) => seq.useCount > (max?.useCount || 0) ? seq : max, undefined);
        const recentlyUsedSequences = allSequences
            .filter(seq => seq.lastUsed)
            .sort((a, b) => (b.lastUsed?.getTime() || 0) - (a.lastUsed?.getTime() || 0))
            .slice(0, 5);
        return {
            totalSequences: allSequences.length,
            activeSequences: activeSequences.length,
            totalUsage,
            mostUsedSequence,
            recentlyUsedSequences,
        };
    }
    // ==================== PERSISTENCE ====================
    /**
     * Save sequences to backend
     */
    async saveSequences() {
        try {
            // In a real implementation, this would save to the backend
            // For now, we'll just simulate the save operation
            console.log('Saving keyboard sequences to backend...');
            // Simulate API delay
            await new Promise(resolve => setTimeout(resolve, 100));
            console.log('Keyboard sequences saved successfully');
        }
        catch (error) {
            console.error('Failed to save keyboard sequences:', error);
            throw error;
        }
    }
    // ==================== UTILITY METHODS ====================
    /**
     * Generate sequence suggestions
     */
    generateSuggestions(vaultName, count = 5) {
        const suggestions = [];
        const baseName = vaultName.toLowerCase().replace(/[^a-z0-9]/g, '');
        // Name-based suggestions
        if (baseName.length >= 2) {
            suggestions.push(`T${baseName}123`);
            suggestions.push(`P${baseName}456`);
            suggestions.push(`${baseName}2024`);
        }
        // Pattern-based suggestions
        const patterns = [
            'Tvault123',
            'Psecure456',
            'Tmy2024',
            'Pdata789',
            'Tfiles321',
        ];
        for (const pattern of patterns) {
            if (suggestions.length >= count)
                break;
            // Check if pattern conflicts
            const conflicts = this.findConflicts(pattern);
            if (conflicts.length === 0) {
                suggestions.push(pattern);
            }
        }
        // Fill remaining with random suggestions
        while (suggestions.length < count) {
            const randomNum = Math.floor(Math.random() * 9000) + 1000;
            const modes = ['T', 'P'];
            const mode = modes[Math.floor(Math.random() * modes.length)];
            const suggestion = `${mode}vault${randomNum}`;
            if (!suggestions.includes(suggestion) && this.findConflicts(suggestion).length === 0) {
                suggestions.push(suggestion);
            }
        }
        return suggestions.slice(0, count);
    }
    /**
     * Parse sequence to extract mode and password
     */
    parseSequence(sequence) {
        if (!sequence || sequence.length < 1) {
            return { mode: 'default', password: '', isValid: false };
        }
        const firstChar = sequence[0].toLowerCase();
        if (firstChar === 't' && sequence.length > 1) {
            return {
                mode: 'temporary',
                password: sequence.slice(1),
                isValid: sequence.length > 1,
            };
        }
        if (firstChar === 'p' && sequence.length > 1) {
            return {
                mode: 'permanent',
                password: sequence.slice(1),
                isValid: sequence.length > 1,
            };
        }
        return {
            mode: 'default',
            password: sequence,
            isValid: sequence.length >= 2,
        };
    }
}
exports.KeyboardSequenceManager = KeyboardSequenceManager;
// ==================== SINGLETON INSTANCE ====================
let globalSequenceManager = null;
/**
 * Get the global keyboard sequence manager instance
 */
function getKeyboardSequenceManager() {
    if (!globalSequenceManager) {
        globalSequenceManager = new KeyboardSequenceManager();
    }
    return globalSequenceManager;
}
/**
 * Initialize the global keyboard sequence manager
 */
async function initializeKeyboardSequenceManager() {
    const manager = getKeyboardSequenceManager();
    await manager.initialize();
    return manager;
}
