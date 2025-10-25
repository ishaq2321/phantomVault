/**
 * Folder Selector Component
 * 
 * Component for selecting folders with validation and preview
 */

import React, { useState, useCallback } from 'react';
import './FolderSelector.css';

interface FolderSelectorProps {
  value: string;
  onChange: (path: string) => void;
  placeholder?: string;
  error?: boolean;
  className?: string;
}

/**
 * Folder selector component
 */
export const FolderSelector: React.FC<FolderSelectorProps> = ({
  value,
  onChange,
  placeholder = 'Select a folder',
  error = false,
  className = '',
}) => {
  const [isSelecting, setIsSelecting] = useState(false);

  // ==================== FOLDER SELECTION ====================

  const handleSelectFolder = useCallback(async () => {
    try {
      setIsSelecting(true);
      
      // Use the PhantomVault API to select a folder
      const selectedPath = await window.phantomVault.selectFolder();
      
      if (selectedPath) {
        onChange(selectedPath);
      }
    } catch (error) {
      console.error('Failed to select folder:', error);
    } finally {
      setIsSelecting(false);
    }
  }, [onChange]);

  const handleClearSelection = useCallback(() => {
    onChange('');
  }, [onChange]);

  // ==================== PATH FORMATTING ====================

  const formatPath = (path: string): string => {
    if (!path) return '';
    
    // Show last 3 parts of the path for readability
    const parts = path.split('/').filter(Boolean);
    if (parts.length > 3) {
      return `.../${parts.slice(-3).join('/')}`;
    }
    return path;
  };

  const getPathInfo = (path: string) => {
    if (!path) return null;
    
    const parts = path.split('/').filter(Boolean);
    const folderName = parts[parts.length - 1] || 'Root';
    
    return {
      name: folderName,
      fullPath: path,
      displayPath: formatPath(path),
    };
  };

  // ==================== VALIDATION ====================

  const validatePath = (path: string): { isValid: boolean; message?: string } => {
    if (!path) {
      return { isValid: false, message: 'Please select a folder' };
    }
    
    // Basic path validation
    if (path.length < 2) {
      return { isValid: false, message: 'Invalid folder path' };
    }
    
    // Check for system directories (basic check)
    const systemPaths = ['/bin', '/boot', '/dev', '/etc', '/lib', '/proc', '/root', '/sbin', '/sys', '/usr/bin', '/usr/sbin'];
    if (systemPaths.some(sysPath => path.startsWith(sysPath))) {
      return { isValid: false, message: 'Cannot select system directories' };
    }
    
    return { isValid: true };
  };

  // ==================== RENDER ====================

  const pathInfo = getPathInfo(value);
  const validation = validatePath(value);

  return (
    <div className={`folder-selector ${className} ${error ? 'error' : ''}`}>
      <div className="selector-container">
        {/* Path Display */}
        <div className="path-display">
          {pathInfo ? (
            <div className="selected-path">
              <div className="path-icon">📁</div>
              <div className="path-info">
                <div className="path-name" title={pathInfo.fullPath}>
                  {pathInfo.name}
                </div>
                <div className="path-location" title={pathInfo.fullPath}>
                  {pathInfo.displayPath}
                </div>
              </div>
              <button
                type="button"
                onClick={handleClearSelection}
                className="clear-button"
                title="Clear selection"
              >
                ✕
              </button>
            </div>
          ) : (
            <div className="empty-selection">
              <div className="empty-icon">📂</div>
              <div className="empty-text">{placeholder}</div>
            </div>
          )}
        </div>

        {/* Select Button */}
        <button
          type="button"
          onClick={handleSelectFolder}
          disabled={isSelecting}
          className="select-button"
        >
          {isSelecting ? (
            <>
              <span className="loading-spinner">⏳</span>
              Selecting...
            </>
          ) : (
            <>
              <span className="button-icon">📁</span>
              {pathInfo ? 'Change' : 'Browse'}
            </>
          )}
        </button>
      </div>

      {/* Validation Message */}
      {value && !validation.isValid && (
        <div className="validation-message error">
          <span className="validation-icon">⚠️</span>
          <span className="validation-text">{validation.message}</span>
        </div>
      )}

      {/* Path Preview */}
      {pathInfo && validation.isValid && (
        <div className="path-preview">
          <div className="preview-label">Full path:</div>
          <div className="preview-path" title={pathInfo.fullPath}>
            {pathInfo.fullPath}
          </div>
        </div>
      )}

      {/* Help Text */}
      <div className="help-text">
        <div className="help-icon">💡</div>
        <div className="help-content">
          Select a folder that you want to encrypt and protect. 
          The folder will be hidden when the vault is locked.
        </div>
      </div>
    </div>
  );
};