/**
 * Password Strength Indicator Component
 * 
 * Visual indicator for password strength with feedback and requirements
 */

import React from 'react';
import { PasswordStrength } from '../../types';

interface PasswordStrengthIndicatorProps {
  strength: PasswordStrength;
  className?: string;
}

/**
 * Password strength indicator component
 */
export const PasswordStrengthIndicator: React.FC<PasswordStrengthIndicatorProps> = ({
  strength,
  className = '',
}) => {
  // ==================== STRENGTH CALCULATION ====================

  const getStrengthLevel = (score: number): 'weak' | 'fair' | 'good' | 'strong' | 'excellent' => {
    if (score <= 1) return 'weak';
    if (score <= 2) return 'fair';
    if (score <= 3) return 'good';
    if (score <= 4) return 'strong';
    return 'excellent';
  };

  const getStrengthColor = (level: string): string => {
    switch (level) {
      case 'weak': return '#F44336';
      case 'fair': return '#FF9800';
      case 'good': return '#FFC107';
      case 'strong': return '#8BC34A';
      case 'excellent': return '#4CAF50';
      default: return '#9E9E9E';
    }
  };

  const getStrengthText = (level: string): string => {
    switch (level) {
      case 'weak': return 'Weak';
      case 'fair': return 'Fair';
      case 'good': return 'Good';
      case 'strong': return 'Strong';
      case 'excellent': return 'Excellent';
      default: return 'Unknown';
    }
  };

  // ==================== RENDER ====================

  const strengthLevel = getStrengthLevel(strength.score);
  const strengthColor = getStrengthColor(strengthLevel);
  const strengthText = getStrengthText(strengthLevel);

  return (
    <div className={`password-strength-indicator ${className}`}>
      {/* Strength Bar */}
      <div className="strength-bar-container">
        <div className="strength-bar">
          {[1, 2, 3, 4, 5].map((level) => (
            <div
              key={level}
              className={`strength-segment ${level <= strength.score ? 'filled' : 'empty'}`}
              style={{
                backgroundColor: level <= strength.score ? strengthColor : '#424769',
              }}
            />
          ))}
        </div>
        <span className="strength-text" style={{ color: strengthColor }}>
          {strengthText}
        </span>
      </div>

      {/* Requirements Checklist */}
      <div className="requirements-checklist">
        <div className={`requirement ${strength.requirements.minLength ? 'met' : 'unmet'}`}>
          <span className="requirement-icon">
            {strength.requirements.minLength ? '✓' : '○'}
          </span>
          <span className="requirement-text">At least 8 characters</span>
        </div>
        
        <div className={`requirement ${strength.requirements.hasUppercase ? 'met' : 'unmet'}`}>
          <span className="requirement-icon">
            {strength.requirements.hasUppercase ? '✓' : '○'}
          </span>
          <span className="requirement-text">Uppercase letter</span>
        </div>
        
        <div className={`requirement ${strength.requirements.hasLowercase ? 'met' : 'unmet'}`}>
          <span className="requirement-icon">
            {strength.requirements.hasLowercase ? '✓' : '○'}
          </span>
          <span className="requirement-text">Lowercase letter</span>
        </div>
        
        <div className={`requirement ${strength.requirements.hasNumbers ? 'met' : 'unmet'}`}>
          <span className="requirement-icon">
            {strength.requirements.hasNumbers ? '✓' : '○'}
          </span>
          <span className="requirement-text">Number</span>
        </div>
        
        <div className={`requirement ${strength.requirements.hasSpecialChars ? 'met' : 'unmet'}`}>
          <span className="requirement-icon">
            {strength.requirements.hasSpecialChars ? '✓' : '○'}
          </span>
          <span className="requirement-text">Special character</span>
        </div>
      </div>

      {/* Feedback Messages */}
      {strength.feedback.length > 0 && (
        <div className="strength-feedback">
          <div className="feedback-title">Suggestions:</div>
          <ul className="feedback-list">
            {strength.feedback.map((message, index) => (
              <li key={index} className="feedback-item">
                {message}
              </li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
};