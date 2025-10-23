import React, { useCallback } from 'react';
import { Icon } from '../common/Icon';
import { Button } from '../common/Button';
import { formatBytes, formatDate } from '../../utils/formatters';
// import './FolderGrid.scss';

interface FolderGridProps {
  folders: Array<{
    id: string;
    name: string;
    path: string;
    isSecure: boolean;
    isLocked: boolean;
    lastModified: Date;
    size: number;
  }>;
  viewMode: 'grid' | 'list';
  onFolderClick: (folderId: string) => void;
  onFolderLock: (folderId: string) => void;
  onFolderUnlock: (folderId: string) => void;
  onFolderDelete: (folderId: string) => void;
}

export const FolderGrid: React.FC<FolderGridProps> = ({
  folders,
  viewMode,
  onFolderClick,
  onFolderLock,
  onFolderUnlock,
  onFolderDelete,
}) => {
  const handleFolderClick = useCallback(
    (event: React.MouseEvent, folderId: string) => {
      event.preventDefault();
      onFolderClick(folderId);
    },
    [onFolderClick]
  );

  const handleLockToggle = useCallback(
    (event: React.MouseEvent, folderId: string, isLocked: boolean) => {
      event.stopPropagation();
      if (isLocked) {
        onFolderUnlock(folderId);
      } else {
        onFolderLock(folderId);
      }
    },
    [onFolderLock, onFolderUnlock]
  );

  const handleDelete = useCallback(
    (event: React.MouseEvent, folderId: string) => {
      event.stopPropagation();
      onFolderDelete(folderId);
    },
    [onFolderDelete]
  );

  return (
    <div className={`folder-grid ${viewMode === 'list' ? 'folder-grid--list' : ''}`}>
      {folders.map((folder) => (
        <div
          key={folder.id}
          className={`folder-grid__item ${
            folder.isSecure ? 'folder-grid__item--secure' : ''
          } ${folder.isLocked ? 'folder-grid__item--locked' : ''}`}
          onClick={(e) => handleFolderClick(e, folder.id)}
        >
          <div className="folder-grid__item-content">
            <Icon
              name={folder.isSecure ? 'folder-secure' : 'folder'}
              className="folder-grid__item-icon"
            />
            <div className="folder-grid__item-name">{folder.name}</div>
            <div className="folder-grid__item-meta">
              <Icon name="clock" />
              <span>{formatDate(folder.lastModified)}</span>
              <Icon name="database" />
              <span>{formatBytes(folder.size)}</span>
            </div>
          </div>

          <div className="folder-grid__item-actions">
            <Button
              variant="icon"
              className="folder-grid__item-actions-button"
              onClick={(e) => handleLockToggle(e, folder.id, folder.isLocked)}
              title={folder.isLocked ? 'Unlock folder' : 'Lock folder'}
            >
              <Icon name={folder.isLocked ? 'lock' : 'unlock'} />
            </Button>
            <Button
              variant="icon"
              className="folder-grid__item-actions-button"
              onClick={(e) => handleDelete(e, folder.id)}
              title="Delete folder"
            >
              <Icon name="trash" />
            </Button>
          </div>

          <div
            className={`folder-grid__item-status folder-grid__item-status--${
              folder.isSecure ? 'secure' : 'warning'
            }`}
          >
            <Icon
              name={folder.isSecure ? 'shield-check' : 'shield-warning'}
              className="folder-grid__item-status-icon"
            />
            <span className="folder-grid__item-status-text">
              {folder.isSecure ? 'Secured' : 'Not Secured'}
            </span>
          </div>
        </div>
      ))}
    </div>
  );
}; 