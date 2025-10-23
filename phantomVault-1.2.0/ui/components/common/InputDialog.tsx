import React, { useState, useEffect, useRef } from 'react';

interface InputDialogProps {
  title: string;
  message: string;
  placeholder?: string;
  defaultValue?: string;
  type?: 'text' | 'password';
  onConfirm: (value: string) => void;
  onCancel: () => void;
}

export const InputDialog: React.FC<InputDialogProps> = ({
  title,
  message,
  placeholder = '',
  defaultValue = '',
  type = 'text',
  onConfirm,
  onCancel,
}) => {
  const [value, setValue] = useState(defaultValue);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    // Auto-focus the input when dialog opens
    inputRef.current?.focus();
  }, []);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (value.trim()) {
      onConfirm(value);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Escape') {
      onCancel();
    }
  };

  return (
    <div
      style={{
        position: 'fixed',
        top: 0,
        left: 0,
        right: 0,
        bottom: 0,
        backgroundColor: 'rgba(0, 0, 0, 0.8)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        zIndex: 2000,
      }}
      onClick={onCancel}
    >
      <div
        style={{
          backgroundColor: '#2D3250',
          borderRadius: '12px',
          padding: '2rem',
          maxWidth: '450px',
          width: '90%',
          boxShadow: '0 20px 60px rgba(0, 0, 0, 0.5)',
        }}
        onClick={(e) => e.stopPropagation()}
      >
        <h2 style={{ margin: '0 0 0.5rem', fontSize: '1.25rem', fontWeight: '600', color: '#F6F6F6' }}>
          {title}
        </h2>
        <p style={{ margin: '0 0 1.5rem', fontSize: '0.875rem', color: '#B4B4B4' }}>
          {message}
        </p>

        <form onSubmit={handleSubmit}>
          <input
            ref={inputRef}
            type={type}
            value={value}
            onChange={(e) => setValue(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder={placeholder}
            style={{
              width: '100%',
              padding: '0.75rem',
              backgroundColor: '#1B1F3B',
              border: '2px solid #424769',
              borderRadius: '8px',
              color: '#F6F6F6',
              fontSize: '1rem',
              outline: 'none',
              marginBottom: '1.5rem',
              boxSizing: 'border-box',
            }}
          />

          <div style={{ display: 'flex', gap: '0.75rem', justifyContent: 'flex-end' }}>
            <button
              type="button"
              onClick={onCancel}
              style={{
                padding: '0.75rem 1.5rem',
                backgroundColor: 'transparent',
                color: '#F6F6F6',
                border: '1px solid #424769',
                borderRadius: '8px',
                fontSize: '0.875rem',
                fontWeight: '500',
                cursor: 'pointer',
              }}
            >
              Cancel
            </button>
            <button
              type="submit"
              disabled={!value.trim()}
              style={{
                padding: '0.75rem 1.5rem',
                backgroundColor: value.trim() ? '#7077A1' : '#424769',
                color: '#F6F6F6',
                border: 'none',
                borderRadius: '8px',
                fontSize: '0.875rem',
                fontWeight: '500',
                cursor: value.trim() ? 'pointer' : 'not-allowed',
              }}
            >
              Confirm
            </button>
          </div>
        </form>
      </div>
    </div>
  );
};
