#!/usr/bin/env node
/**
 * Cleanup Old Folders - Removes old-style folders from metadata
 * 
 * This removes folders that don't have the new vault metadata structure
 * (originalPath, vaultPath, backups[])
 * 
 * IMPORTANT: Run this AFTER manually unlocking any important old folders!
 */

const fs = require('fs');
const path = require('path');
const os = require('os');

const username = os.userInfo().username;
const vaultStorageBase = path.join(os.homedir(), '.phantom_vault_storage', username);

console.log('\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
console.log('🧹 PhantomVault: Old Folder Cleanup Utility');
console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');

// Find metadata file in profile directory
let metadataPath = null;
if (fs.existsSync(vaultStorageBase)) {
  const entries = fs.readdirSync(vaultStorageBase);
  for (const entry of entries) {
    if (entry.startsWith('profile_')) {
      const testPath = path.join(vaultStorageBase, entry, 'folders_metadata.json');
      if (fs.existsSync(testPath)) {
        metadataPath = testPath;
        break;
      }
    }
  }
}

if (!metadataPath) {
  console.log('❌ Metadata file not found in:', vaultStorageBase);
  console.log('   No cleanup needed.\n');
  process.exit(0);
}

console.log('📂 Found metadata:', metadataPath);

// Load metadata
const metadata = JSON.parse(fs.readFileSync(metadataPath, 'utf8'));

console.log('📊 Current folder count:', metadata.folders.length);

// Separate old and new folders
const oldFolders = metadata.folders.filter(f => !f.vaultPath || !f.originalPath);
const newFolders = metadata.folders.filter(f => f.vaultPath && f.originalPath);

console.log('   Old system folders:', oldFolders.length);
console.log('   New vault folders:', newFolders.length);

if (oldFolders.length === 0) {
  console.log('\n✅ No old folders found. All folders use the new vault system!\n');
  process.exit(0);
}

console.log('\n📋 Old folders to be removed:\n');
oldFolders.forEach((folder, index) => {
  console.log(`   ${index + 1}. ${folder.folderName}`);
  console.log(`      ID: ${folder.id}`);
  console.log(`      Path: ${folder.folderPath}`);
  console.log(`      Locked: ${folder.isLocked}`);
  console.log('');
});

// Create backup
const backupPath = metadataPath + '.backup.' + Date.now();
fs.copyFileSync(metadataPath, backupPath);
console.log('💾 Backup created:', backupPath);

// Update metadata to only keep new folders
metadata.folders = newFolders;
metadata.lastModified = Date.now();

// Save updated metadata
fs.writeFileSync(metadataPath, JSON.stringify(metadata, null, 2), { mode: 0o600 });

console.log('\n✅ Cleanup complete!');
console.log(`   Removed: ${oldFolders.length} old folder(s)`);
console.log(`   Remaining: ${newFolders.length} vault folder(s)`);
console.log('\n⚠️  Important: If any removed folders contained important data,');
console.log('   restore from backup:', path.basename(backupPath));
console.log('\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');
