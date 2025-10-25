import React, { forwardRef } from 'react';
import { Icon } from './Icon';
import './Input.css';

interface InputProps {
  value: string;
  onChange: (value: string) => void;
  type?: 'text' | 'password' | 'email' | 'number' | 'search';
  placeholder?: string;
  label?: string;
  error?: string;
  disabled?: boolean;
  icon?: string;
  iconPosition?: 'left' | 'right';
  showPasswordToggle?: boolean;
  fullWidth?: boolean;
  className?: string;
  onEnter?: () => void;
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  (
    {
      value,
      onChange,
      type = 'text',
      placeholder,
      label,
      error,
      disabled = false,
      icon,
      iconPosition = 'left',
      showPasswordToggle = false,
      fullWidth = false,
      className = '',
      onEnter,
    },
    ref
  ) => {
    const [showPassword, setShowPassword] = React.useState(false);
    const inputType = type === 'password' && showPassword ? 'text' : type;

    const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
      if (e.key === 'Enter' && onEnter) {
        onEnter();
      }
    };

    return (
      <div className={`input-wrapper ${fullWidth ? 'input-wrapper--full-width' : ''} ${className}`}>
        {label && <label className="input__label">{label}</label>}
        
        <div className={`input-container ${error ? 'input-container--error' : ''}`}>
          {icon && iconPosition === 'left' && (
            <Icon name={icon} size="small" className="input__icon input__icon--left" />
          )}
          
          <input
            ref={ref}
            type={inputType}
            value={value}
            onChange={(e) => onChange(e.target.value)}
            placeholder={placeholder}
            disabled={disabled}
            className={`input ${icon && iconPosition === 'left' ? 'input--with-left-icon' : ''} ${
              (icon && iconPosition === 'right') || (type === 'password' && showPasswordToggle)
                ? 'input--with-right-icon'
                : ''
            }`}
            onKeyDown={handleKeyDown}
          />
          
          {type === 'password' && showPasswordToggle && (
            <button
              type="button"
              className="input__toggle-password"
              onClick={() => setShowPassword(!showPassword)}
              tabIndex={-1}
            >
              <Icon name={showPassword ? 'unlock' : 'lock'} size="small" />
            </button>
          )}
          
          {icon && iconPosition === 'right' && !(type === 'password' && showPasswordToggle) && (
            <Icon name={icon} size="small" className="input__icon input__icon--right" />
          )}
        </div>
        
        {error && <span className="input__error">{error}</span>}
      </div>
    );
  }
);

Input.displayName = 'Input';
