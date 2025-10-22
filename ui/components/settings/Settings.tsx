import React, { useState, useEffect } from 'react';

interface SettingsProps {
  onClose: () => void;
}

export const Settings: React.FC<SettingsProps> = ({ onClose }) => {
  const [hotkey, setHotkey] = useState<string>('CommandOrControl+Shift+P');
  const [isRecording, setIsRecording] = useState(false);
  const [recordedKeys, setRecordedKeys] = useState<string[]>([]);

  useEffect(() => {
    // Load saved hotkey from storage
    const loadHotkey = async () => {
      try {
        const saved = localStorage.getItem('phantomvault_hotkey');
        if (saved) {
          setHotkey(saved);
        }
      } catch (err) {
        console.error('Failed to load hotkey:', err);
      }
    };
    loadHotkey();
  }, []);

  const handleStartRecording = () => {
    setIsRecording(true);
    setRecordedKeys([]);
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (!isRecording) return;
    
    e.preventDefault();
    
    const keys: string[] = [];
    
    if (e.ctrlKey || e.metaKey) keys.push('CommandOrControl');
    if (e.shiftKey) keys.push('Shift');
    if (e.altKey) keys.push('Alt');
    
    // Get the actual key
    const key = e.key.toUpperCase();
    if (key !== 'CONTROL' && key !== 'SHIFT' && key !== 'ALT' && key !== 'META') {
      keys.push(key);
    }
    
    if (keys.length > 1) { // Need at least modifier + key
      const hotkeyString = keys.join('+');
      setRecordedKeys(keys);
      setHotkey(hotkeyString);
      setIsRecording(false);
      
      // Save to storage
      localStorage.setItem('phantomvault_hotkey', hotkeyString);
      
      // Notify Electron to re-register the hotkey
      // TODO: Implement registerGlobalHotkey in the API
      // window.phantomVault.registerGlobalHotkey(hotkeyString)
      //   .then(() => {
          window.phantomVault.showNotification(
            'Hotkey Updated',
            `New hotkey: ${hotkeyString}`
          );
      //   })
        .catch((err: any) => {
          console.error('Failed to register hotkey:', err);
          window.phantomVault.showNotification(
            'Error',
            'Failed to register hotkey. It may be in use by another application.'
          );
        });
    }
  };

  return (
    <div style={{
      position: 'fixed',
      top: 0,
      left: 0,
      right: 0,
      bottom: 0,
      backgroundColor: 'rgba(0, 0, 0, 0.8)',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      zIndex: 1000,
    }}>
      <div style={{
        backgroundColor: '#2D3250',
        borderRadius: '12px',
        padding: '2rem',
        maxWidth: '500px',
        width: '90%',
        boxShadow: '0 20px 60px rgba(0, 0, 0, 0.5)',
      }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '2rem' }}>
          <h2 style={{ margin: 0, fontSize: '1.5rem', fontWeight: '600', color: '#F6F6F6' }}>
            ⚙️ Settings
          </h2>
          <button onClick={onClose} style={{
            background: 'none',
            border: 'none',
            color: '#F6F6F6',
            fontSize: '1.5rem',
            cursor: 'pointer',
            padding: '0.25rem',
          }}>
            ✕
          </button>
        </div>

        <div style={{ marginBottom: '2rem' }}>
          <h3 style={{ fontSize: '1rem', fontWeight: '600', marginBottom: '1rem', color: '#F6F6F6' }}>
            Global Unlock Hotkey
          </h3>
          <p style={{ fontSize: '0.875rem', color: '#B4B4B4', marginBottom: '1rem' }}>
            Press this keyboard combination from anywhere to unlock your vaults
          </p>

          <div style={{
            padding: '1rem',
            backgroundColor: '#1B1F3B',
            borderRadius: '8px',
            border: '2px solid #424769',
            marginBottom: '1rem',
            textAlign: 'center',
          }}>
            <div style={{ fontSize: '1.25rem', fontWeight: '600', color: '#7077A1', marginBottom: '0.5rem' }}>
              {hotkey}
            </div>
            <div style={{ fontSize: '0.75rem', color: '#B4B4B4' }}>
              Current hotkey
            </div>
          </div>

          <div
            onKeyDown={handleKeyPress}
            tabIndex={0}
            style={{
              padding: '1.5rem',
              backgroundColor: isRecording ? '#424769' : '#1B1F3B',
              border: `2px dashed ${isRecording ? '#7077A1' : '#424769'}`,
              borderRadius: '8px',
              textAlign: 'center',
              cursor: 'pointer',
              outline: 'none',
            }}
            onClick={handleStartRecording}
          >
            {isRecording ? (
              <>
                <div style={{ fontSize: '1rem', color: '#F6F6F6', marginBottom: '0.5rem' }}>
                  ⌨️ Press your desired key combination...
                </div>
                {recordedKeys.length > 0 && (
                  <div style={{ fontSize: '1.25rem', fontWeight: '600', color: '#7077A1' }}>
                    {recordedKeys.join(' + ')}
                  </div>
                )}
              </>
            ) : (
              <div style={{ fontSize: '1rem', color: '#B4B4B4' }}>
                Click here to change hotkey
              </div>
            )}
          </div>

          <div style={{
            marginTop: '1rem',
            padding: '0.75rem',
            backgroundColor: '#1B1F3B',
            borderRadius: '8px',
            fontSize: '0.75rem',
            color: '#B4B4B4',
          }}>
            💡 <strong>Tip:</strong> Use a combination like Ctrl+Shift+P or Ctrl+Alt+V
          </div>
        </div>

        <div style={{ display: 'flex', justifyContent: 'flex-end' }}>
          <button onClick={onClose} style={{
            padding: '0.75rem 2rem',
            backgroundColor: '#7077A1',
            color: '#F6F6F6',
            border: 'none',
            borderRadius: '8px',
            fontSize: '1rem',
            fontWeight: '500',
            cursor: 'pointer',
          }}>
            Done
          </button>
        </div>
      </div>
    </div>
  );
};
