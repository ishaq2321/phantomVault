// import React from 'react';
import ReactDOM from 'react-dom/client';
import { App } from './App';
// import '../styles/design_system.scss'; // Temporarily disabled
import './index.css';

// Verify that the Electron API is available
if (!window.phantomVault) {
  console.error('PhantomVault API not found! Make sure preload script is loaded.');
} else {
  console.log('✅ PhantomVault API is available in main.tsx');
  console.log('📋 Has onShowUnlockOverlay?', typeof window.phantomVault.onShowUnlockOverlay);
}

// Log version on startup
window.phantomVault?.getVersion().then((version) => {
  console.log(`🔐 PhantomVault v${version} - Starting...`);
}).catch((error) => {
  console.error('Failed to get version:', error);
});

// Mount React app
ReactDOM.createRoot(document.getElementById('root')!).render(
  <App />
);
