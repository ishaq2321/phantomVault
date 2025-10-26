/**
 * PhantomVault App Theme Hook
 */

import { useState, useCallback } from 'react';

export const useAppTheme = () => {
  const [theme, setTheme] = useState<'light' | 'dark'>('dark');

  const toggleTheme = useCallback(() => {
    setTheme(prev => prev === 'light' ? 'dark' : 'light');
  }, []);

  return { theme, toggleTheme };
};