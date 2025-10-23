#!/usr/bin/env node
/**
 * Remove Old Encrypted Folders
 * 
 * Removes old-style encrypted folders (with dot prefix) that are no longer in metadata
 * 
 * DANGEROUS: This will permanently delete folders. Only run if you're sure they're old encrypted folders!
 */

const fs = require('fs');
const path = require('path');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

const oldFolders = [
  '/home/ishaq2321/Desktop/.Test 2',
  '/home/ishaq2321/Desktop/ML/.Automated Content Categorization for Digital Communications',
  '/home/ishaq2321/Desktop/ML/.Automated Content Categorization'
];

console.log('\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
console.log('🗑️  PhantomVault: Remove Old Encrypted Folders');
console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');

console.log('⚠️  WARNING: This will PERMANENTLY DELETE the following folders:\n');

oldFolders.forEach((folder, index) => {
  const exists = fs.existsSync(folder);
  console.log(`   ${index + 1}. ${folder}`);
  console.log(`      Exists: ${exists ? '✅ Yes' : '❌ No'}`);
  if (exists) {
    const stats = fs.statSync(folder);
    console.log(`      Type: ${stats.isDirectory() ? 'Directory' : 'File'}`);
  }
  console.log('');
});

rl.question('Do you want to DELETE these folders? (yes/no): ', (answer) => {
  if (answer.toLowerCase() === 'yes') {
    console.log('\n🗑️  Deleting folders...\n');
    
    let deletedCount = 0;
    oldFolders.forEach(folder => {
      if (fs.existsSync(folder)) {
        try {
          fs.rmSync(folder, { recursive: true, force: true });
          console.log(`   ✅ Deleted: ${folder}`);
          deletedCount++;
        } catch (error) {
          console.log(`   ❌ Failed: ${folder} - ${error.message}`);
        }
      } else {
        console.log(`   ⏭️  Skipped: ${folder} (doesn't exist)`);
      }
    });
    
    console.log(`\n✅ Cleanup complete! Deleted ${deletedCount} folder(s)\n`);
  } else {
    console.log('\n❌ Cancelled. No folders deleted.\n');
  }
  
  rl.close();
});
