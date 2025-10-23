/**
 * Vault Status Animation Component
 * 
 * Provides visual feedback for vault status changes with smooth animations
 */

import React, { useState, useEffect } from 'react';
import { VaultStatus } from '../../types';
import { VaultStatusChange } from '../../services/VaultStatusMonitor';

interface VaultStatusAnimationProps {
  vaultId: string;
  currentStatus: VaultStatus;
  statusChange?: VaultStatusChange;
  className?: string;
}

/**
 * Status animation component
 */
export const VaultStatusAnimation: React.FC<VaultStatusAnimationProps> = ({
  vaultId,
  currentStatus,
  statusChange,
  className = '',
}) => {
  const [isAnimating, setIsAnimating] = useState(false);
  const [animationType, setAnimationType] = useState<'pulse' | 'slide' | 'fade' | 'bounce'>('pulse');

  // ==================== ANIMATION LOGIC ====================

  useEffect(() => {
    if (statusChange && statusChange.vaultId === vaultId) {
      // Determine animation type based on status change
      const newAnimationType = getAnimationTypeForStatusChange(
        statusChange.oldStatus,
        statusChange.newStatus
      );
      
      setAnimationType(newAnimationType);
      setIsAnimating(true);

      // Stop animation after duration
      const animationDuration = getAnimationDuration(newAnimationType);
      const timer = setTimeout(() => {
        setIsAnimating(false);
      }, animationDuration);

      return () => clearTimeout(timer);
    }
  }, [statusChange, vaultId]);

  // ==================== ANIMATION HELPERS ====================

  const getAnimationTypeForStatusChange = (
    oldStatus: VaultStatus,
    newStatus: VaultStatus
  ): 'pulse' | 'slide' | 'fade' | 'bounce' => {
    // Mount/unmount operations
    if (oldStatus === 'unmounted' && newStatus === 'mounted') return 'bounce';
    if (oldStatus === 'mounted' && newStatus === 'unmounted') return 'slide';
    
    // Error states
    if (newStatus === 'error') return 'pulse';
    
    // Loading states
    if (['loading', 'encrypting', 'decrypting'].includes(newStatus)) return 'pulse';
    
    // Default
    return 'fade';
  };

  const getAnimationDuration = (type: string): number => {
    switch (type) {
      case 'bounce': return 800;
      case 'slide': return 600;
      case 'pulse': return 1200;
      case 'fade': return 400;
      default: return 600;
    }
  };

  const getStatusIcon = (status: VaultStatus): string => {
    switch (status) {
      case 'mounted': return '🔓';
      case 'unmounted': return '🔒';
      case 'error': return '❌';
      case 'loading': return '⏳';
      case 'encrypting': return '🔐';
      case 'decrypting': return '🔑';
      default: return '❓';
    }
  };

  const getStatusColor = (status: VaultStatus): string => {
    switch (status) {
      case 'mounted': return '#4CAF50';
      case 'unmounted': return '#FF9800';
      case 'error': return '#F44336';
      case 'loading': 
      case 'encrypting': 
      case 'decrypting': return '#2196F3';
      default: return '#9E9E9E';
    }
  };

  // ==================== RENDER ====================

  const baseClasses = `vault-status-animation ${className}`;
  const animationClasses = isAnimating ? `animating animation-${animationType}` : '';
  const statusClasses = `status-${currentStatus}`;

  return (
    <div 
      className={`${baseClasses} ${animationClasses} ${statusClasses}`}
      style={{
        '--status-color': getStatusColor(currentStatus),
      } as React.CSSProperties}
    >
      <div className="status-icon-container">
        <span className="status-icon">{getStatusIcon(currentStatus)}</span>
        
        {/* Animation overlay effects */}
        {isAnimating && (
          <>
            {animationType === 'pulse' && (
              <div className="pulse-ring" />
            )}
            {animationType === 'bounce' && (
              <div className="bounce-effect" />
            )}
            {animationType === 'slide' && (
              <div className="slide-effect" />
            )}
          </>
        )}
      </div>
      
      {/* Status change notification */}
      {isAnimating && statusChange && (
        <div className="status-change-notification">
          <span className="change-text">
            {getStatusChangeMessage(statusChange.oldStatus, statusChange.newStatus)}
          </span>
        </div>
      )}
    </div>
  );
};

// ==================== HELPER FUNCTIONS ====================

function getStatusChangeMessage(oldStatus: VaultStatus, newStatus: VaultStatus): string {
  if (oldStatus === 'unmounted' && newStatus === 'mounted') {
    return 'Mounted';
  }
  if (oldStatus === 'mounted' && newStatus === 'unmounted') {
    return 'Unmounted';
  }
  if (newStatus === 'error') {
    return 'Error';
  }
  if (newStatus === 'loading') {
    return 'Loading...';
  }
  if (newStatus === 'encrypting') {
    return 'Encrypting...';
  }
  if (newStatus === 'decrypting') {
    return 'Decrypting...';
  }
  
  return 'Status Changed';
}

// ==================== CSS STYLES ====================

const styles = `
.vault-status-animation {
  position: relative;
  display: inline-block;
}

.status-icon-container {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.status-icon {
  font-size: 1.5rem;
  transition: all 0.3s ease;
  z-index: 2;
  position: relative;
}

/* Animation Effects */
.animating .status-icon {
  animation-fill-mode: both;
}

/* Pulse Animation */
.animation-pulse .status-icon {
  animation: statusPulse 1.2s ease-in-out;
}

.pulse-ring {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 100%;
  height: 100%;
  border: 2px solid var(--status-color);
  border-radius: 50%;
  animation: pulseRing 1.2s ease-out;
  z-index: 1;
}

@keyframes statusPulse {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.1); }
}

@keyframes pulseRing {
  0% {
    transform: translate(-50%, -50%) scale(0.8);
    opacity: 1;
  }
  100% {
    transform: translate(-50%, -50%) scale(2);
    opacity: 0;
  }
}

/* Bounce Animation */
.animation-bounce .status-icon {
  animation: statusBounce 0.8s ease-out;
}

.bounce-effect {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 120%;
  height: 120%;
  background: radial-gradient(circle, var(--status-color) 0%, transparent 70%);
  border-radius: 50%;
  animation: bounceGlow 0.8s ease-out;
  z-index: 1;
  opacity: 0.3;
}

@keyframes statusBounce {
  0% { transform: scale(1) translateY(0); }
  30% { transform: scale(1.2) translateY(-10px); }
  60% { transform: scale(0.9) translateY(5px); }
  100% { transform: scale(1) translateY(0); }
}

@keyframes bounceGlow {
  0% { transform: translate(-50%, -50%) scale(0.5); opacity: 0.6; }
  50% { transform: translate(-50%, -50%) scale(1.2); opacity: 0.3; }
  100% { transform: translate(-50%, -50%) scale(1.5); opacity: 0; }
}

/* Slide Animation */
.animation-slide .status-icon {
  animation: statusSlide 0.6s ease-in-out;
}

.slide-effect {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent 0%, var(--status-color) 50%, transparent 100%);
  animation: slideEffect 0.6s ease-in-out;
  z-index: 1;
  opacity: 0.4;
}

@keyframes statusSlide {
  0% { transform: translateX(0); }
  50% { transform: translateX(-5px); }
  100% { transform: translateX(0); }
}

@keyframes slideEffect {
  0% { transform: translateX(-100%); }
  100% { transform: translateX(100%); }
}

/* Fade Animation */
.animation-fade .status-icon {
  animation: statusFade 0.4s ease-in-out;
}

@keyframes statusFade {
  0% { opacity: 1; }
  50% { opacity: 0.3; }
  100% { opacity: 1; }
}

/* Status Change Notification */
.status-change-notification {
  position: absolute;
  top: -30px;
  left: 50%;
  transform: translateX(-50%);
  background: var(--status-color);
  color: white;
  padding: 0.25rem 0.75rem;
  border-radius: 12px;
  font-size: 0.75rem;
  font-weight: 500;
  white-space: nowrap;
  animation: notificationSlideIn 0.3s ease-out, notificationFadeOut 0.3s ease-in 2.7s;
  z-index: 10;
}

@keyframes notificationSlideIn {
  0% {
    opacity: 0;
    transform: translateX(-50%) translateY(10px);
  }
  100% {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
  }
}

@keyframes notificationFadeOut {
  0% { opacity: 1; }
  100% { opacity: 0; }
}

/* Status-specific colors */
.status-mounted {
  --status-color: #4CAF50;
}

.status-unmounted {
  --status-color: #FF9800;
}

.status-error {
  --status-color: #F44336;
}

.status-loading,
.status-encrypting,
.status-decrypting {
  --status-color: #2196F3;
}

/* Loading spinner for loading states */
.status-loading .status-icon,
.status-encrypting .status-icon,
.status-decrypting .status-icon {
  animation: spin 2s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}
`;

// Inject styles
if (typeof document !== 'undefined') {
  const styleSheet = document.createElement('style');
  styleSheet.textContent = styles;
  document.head.appendChild(styleSheet);
}