/**
 * Toast Notification Component
 * 
 * Individual toast notification for operation feedback
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NotificationConfig } from '../../types';

export interface ToastNotificationProps {
  notification: NotificationConfig;
  onDismiss: () => void;
  position?: 'top-right' | 'top-left' | 'bottom-right' | 'bottom-left';
}

/**
 * Toast notification component
 */
export const ToastNotification: React.FC<ToastNotificationProps> = ({
  notification,
  onDismiss,
  position = 'top-right',
}) => {
  const [isVisible, setIsVisible] = useState(false);
  const [isExiting, setIsExiting] = useState(false);

  // ==================== EFFECTS ====================

  useEffect(() => {
    // Animate in
    const timer = setTimeout(() => setIsVisible(true), 50);
    return () => clearTimeout(timer);
  }, []);

  useEffect(() => {
    // Auto-dismiss if duration is set
    if (notification.duration && notification.duration > 0) {
      const timer = setTimeout(() => {
        handleDismiss();
      }, notification.duration);
      return () => clearTimeout(timer);
    }
  }, [notification.duration]);

  // ==================== HANDLERS ====================

  const handleDismiss = useCallback(() => {
    setIsExiting(true);
    setTimeout(() => {
      onDismiss();
    }, 300); // Match exit animation duration
  }, [onDismiss]);

  const handleActionClick = useCallback((action: () => void) => {
    action();
    handleDismiss();
  }, [handleDismiss]);

  // ==================== RENDER HELPERS ====================

  const getNotificationIcon = () => {
    switch (notification.type) {
      case 'success': return '✅';
      case 'error': return '❌';
      case 'warning': return '⚠️';
      case 'info': return 'ℹ️';
      default: return '📢';
    }
  };

  const getNotificationClass = () => {
    const baseClass = 'toast-notification';
    const typeClass = `toast-${notification.type}`;
    const positionClass = `toast-${position}`;
    const visibilityClass = isVisible ? 'toast-visible' : 'toast-hidden';
    const exitClass = isExiting ? 'toast-exiting' : '';
    
    return `${baseClass} ${typeClass} ${positionClass} ${visibilityClass} ${exitClass}`.trim();
  };

  const formatTimestamp = (timestamp: Date) => {
    return timestamp.toLocaleTimeString([], { 
      hour: '2-digit', 
      minute: '2-digit',
      second: '2-digit'
    });
  };

  // ==================== MAIN RENDER ====================

  return (
    <div className={getNotificationClass()}>
      {/* Header */}
      <div className="toast-header">
        <div className="toast-icon-title">
          <span className="toast-icon">{getNotificationIcon()}</span>
          <h4 className="toast-title">{notification.title}</h4>
        </div>
        <div className="toast-controls">
          <span className="toast-timestamp">
            {formatTimestamp(notification.timestamp)}
          </span>
          <button
            onClick={handleDismiss}
            className="toast-close"
            title="Dismiss notification"
          >
            ✕
          </button>
        </div>
      </div>

      {/* Content */}
      <div className="toast-content">
        <p className="toast-message">{notification.message}</p>
        
        {/* Progress Bar for Auto-dismiss */}
        {notification.duration && notification.duration > 0 && (
          <div className="toast-progress">
            <div 
              className="toast-progress-bar"
              style={{
                animationDuration: `${notification.duration}ms`,
              }}
            />
          </div>
        )}
      </div>

      {/* Actions */}
      {notification.actions && notification.actions.length > 0 && (
        <div className="toast-actions">
          {notification.actions.map((action, index) => (
            <button
              key={index}
              onClick={() => handleActionClick(action.action)}
              className="toast-action-button"
            >
              {action.label}
            </button>
          ))}
        </div>
      )}

      {/* Details */}
      {notification.details && (
        <div className="toast-details">
          <details className="toast-details-toggle">
            <summary className="toast-details-summary">
              Show Details
            </summary>
            <div className="toast-details-content">
              {typeof notification.details === 'string' ? (
                <p>{notification.details}</p>
              ) : (
                <pre>{JSON.stringify(notification.details, null, 2)}</pre>
              )}
            </div>
          </details>
        </div>
      )}
    </div>
  );
};