import React from 'react';
import { Icon } from './Icon';
// import './Button.scss';

interface ButtonProps {
  children: React.ReactNode;
  onClick?: () => void;
  variant?: 'primary' | 'secondary' | 'outline' | 'critical';
  size?: 'small' | 'medium' | 'large';
  disabled?: boolean;
  loading?: boolean;
  icon?: string;
  iconPosition?: 'left' | 'right';
  fullWidth?: boolean;
  className?: string;
  type?: 'button' | 'submit' | 'reset';
}

export const Button: React.FC<ButtonProps> = ({
  children,
  onClick,
  variant = 'primary',
  size = 'medium',
  disabled = false,
  loading = false,
  icon,
  iconPosition = 'left',
  fullWidth = false,
  className = '',
  type = 'button',
}) => {
  const handleClick = () => {
    if (!disabled && !loading && onClick) {
      onClick();
    }
  };

  return (
    <button
      type={type}
      className={`button button--${variant} button--${size} ${
        fullWidth ? 'button--full-width' : ''
      } ${disabled || loading ? 'button--disabled' : ''} ${className}`}
      onClick={handleClick}
      disabled={disabled || loading}
    >
      {loading && (
        <span className="button__loading">
          <Icon name="loading" size="small" className="button__loading-icon" />
        </span>
      )}
      
      {!loading && icon && iconPosition === 'left' && (
        <Icon name={icon} size="small" className="button__icon button__icon--left" />
      )}
      
      <span className="button__text">{children}</span>
      
      {!loading && icon && iconPosition === 'right' && (
        <Icon name={icon} size="small" className="button__icon button__icon--right" />
      )}
    </button>
  );
};
