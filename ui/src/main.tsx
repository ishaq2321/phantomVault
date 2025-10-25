// import React from 'react';
import ReactDOM from 'react-dom/client';
import { App } from './App';
import { Providers } from './contexts';
// import '../styles/design_system.scss'; // Temporarily disabled
import './index.css';

console.log('🚀 main.tsx loaded');

// Verify that the Electron API is available
if (!window.phantomVault) {
  console.error('❌ PhantomVault API not found! Make sure preload script is loaded.');
  document.body.innerHTML = `
    <div style="display: flex; align-items: center; justify-content: center; height: 100vh; color: #ff6b6b; font-family: monospace; flex-direction: column; gap: 20px;">
      <h1>⚠️ PhantomVault API Not Found</h1>
      <p>The preload script failed to load. Check console for details.</p>
    </div>
  `;
} else {
  console.log('✅ PhantomVault API is available in main.tsx');
  console.log('📋 Has onShowUnlockOverlay?', typeof window.phantomVault.onShowUnlockOverlay);
  
  // Log version on startup
  window.phantomVault?.getVersion().then((version) => {
    console.log(`🔐 PhantomVault v${version} - Starting...`);
  }).catch((error) => {
    console.error('Failed to get version:', error);
  });

  try {
    console.log('🎨 Mounting React app...');
    const rootElement = document.getElementById('root');
    if (!rootElement) {
      throw new Error('Root element not found!');
    }
    console.log('✅ Root element found:', rootElement);
    
    const root = ReactDOM.createRoot(rootElement);
    console.log('✅ React root created');
    
    // Wrap App with Providers to provide VaultContext and AppContext
    root.render(
      <Providers>
        <App />
      </Providers>
    );
    console.log('✅ App component rendered with Providers');
  } catch (error) {
    console.error('❌ Failed to mount React app:', error);
    document.body.innerHTML = `
      <div style="display: flex; align-items: center; justify-content: center; height: 100vh; color: #ff6b6b; font-family: monospace; flex-direction: column; gap: 20px;">
        <h1>⚠️ React Mount Failed</h1>
        <p>${error instanceof Error ? error.message : String(error)}</p>
        <pre style="background: #2a2a2a; padding: 20px; border-radius: 8px;">${error instanceof Error ? error.stack : ''}</pre>
      </div>
    `;
  }
}
