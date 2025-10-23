/**
 * Notification Container Component
 * 
 * Container for displaying toast notifications and alerts
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NotificationConfig } from '../../types';
import { useApp } from '../../contexts';

export interface NotificationContainerProps {
  position?: 'top-right' | 'top-left' | 'bottom-right' | 'bottom-left' | 'center';
  maxNotifications?: number;
  className?: string;
}

/**
 * Individual notification component
 */
const NotificationItem: React.FC<{
  notification: NotificationConfig;
  onClose: (id: string) => void;
}> = ({ notification, onClose }) => {
  const [isVisible, setIsVisible] = useState(false);
  const [isClosing, setIsClosing] = useState(false);

  // Animate in
  useEffect(() => {
    const timer = setTimeout(() => setIsVisible(true), 10);
    return () => clearTimeout(timer);
  }, []);

  // Auto-close after duration
  useEffect(() => {
    if (notification.duration && notification.duration > 0) {
      const timer = setTimeout(() => {
        handleClose();
      }, notification.duration);
      
      return () => clearTimeout(timer);
    }
  }, [notification.duration]);

  // Handle close with animation
  const handleClose = useCallback(() => {
    setIsClosing(true);
    setTimeout(() => {
      onClose(notification.id);
    }, 300); // Match CSS animation duration
  }, [notification.id, onClose]);

  // Get notification icon
  const getIcon = () => {
    switch (notification.type) {
      case 'success': return '✅';
      case 'error': return '❌';
      case 'warning': return '⚠️';
      case 'info': return 'ℹ️';
      default: return 'ℹ️';
    }
  };

  // Get notification color
  const getColor = () => {
    switch (notification.type) {
      case 'success': return '#4CAF50';
      case 'error': return '#F44336';
      case 'warning': return '#FF9800';
      case 'info': return '#2196F3';
      default: return '#2196F3';
    }
  };

  return (
    <div 
      className={`notification-item ${notification.type} ${isVisible ? 'visible' : ''} ${isClosing ? 'closing' : ''}`}
      style={{ borderLeftColor: getColor() }}
    >
      <div className="notification-content">
        <div className="notification-header">
          <span className="notification-icon">{getIcon()}</span>
          <h4 className="notification-title">{notification.title}</h4>
          <button 
            onClick={handleClose}
            className="notification-close\"
            title="Close notification\"
          >
            ✕
          </button>
        </div>
        
        <p className="notification-message">{notification.message}</p>
        
        {notification.actions && notification.actions.length > 0 && (
          <div className="notification-actions">
            {notification.actions.map((action, index) => (
              <button
                key={index}
                onClick={() => {
                  action.action();
                  handleClose();
                }}
                className="notification-action-button\"
              >
                {action.label}
              </button>
            ))}
          </div>
        )}
      </div>
      
      {notification.duration && notification.duration > 0 && (
        <div 
          className="notification-progress\"
          style={{ 
            animationDuration: `${notification.duration}ms`,
            backgroundColor: getColor()
          }}
        />
      )}
    </div>
  );
};

/**
 * Notification container component
 */
export const NotificationContainer: React.FC<NotificationContainerProps> = ({
  position = 'top-right',
  maxNotifications = 5,
  className = '',
}) => {
  const { state: appState, actions: appActions } = useApp();
  const [notifications, setNotifications] = useState<NotificationConfig[]>([]);

  // Sync with app state
  useEffect(() => {
    setNotifications(appState.notifications.slice(0, maxNotifications));
  }, [appState.notifications, maxNotifications]);

  // Handle notification close
  const handleClose = useCallback((id: string) => {
    appActions.removeNotification(id);
  }, [appActions]);

  // Handle clear all notifications
  const handleClearAll = useCallback(() => {
    notifications.forEach(notification => {
      appActions.removeNotification(notification.id);
    });
  }, [notifications, appActions]);

  // Listen for app notification events
  useEffect(() => {
    const handleAppNotification = (event: CustomEvent) => {
      const notification = event.detail;
      appActions.addNotification(notification);
    };

    window.addEventListener('app-notification', handleAppNotification as EventListener);
    
    return () => {
      window.removeEventListener('app-notification', handleAppNotification as EventListener);
    };
  }, [appActions]);

  if (notifications.length === 0) {
    return null;
  }

  return (
    <div className={`notification-container ${position} ${className}`}>
      {/* Clear All Button */}
      {notifications.length > 1 && (
        <div className="notification-header-actions">
          <button 
            onClick={handleClearAll}
            className="clear-all-button\"
            title="Clear all notifications\"
          >
            Clear All ({notifications.length})
          </button>
        </div>
      )}

      {/* Notifications */}
      <div className="notifications-list">
        {notifications.map(notification => (
          <NotificationItem
            key={notification.id}
            notification={notification}
            onClose={handleClose}
          />
        ))}
      </div>
    </div>
  );
};