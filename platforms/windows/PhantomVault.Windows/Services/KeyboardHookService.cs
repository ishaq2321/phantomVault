using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Input;

namespace PhantomVault.Windows.Services;

public class KeyboardHookService : IKeyboardHookService, IDisposable
{
    private readonly Dictionary<(ModifierKeys, Key), int> _hotKeyIds = new();
    private readonly Dictionary<int, Action> _callbacks = new();
    private int _nextHotKeyId = 1;
    private bool _disposed = false;

    public KeyboardHookService()
    {
        // Register window procedure for hot key messages
        Application.Current.MainWindow?.AddHandler(Keyboard.KeyDownEvent, new KeyEventHandler(OnKeyDown), true);
    }

    public void RegisterHotKey(ModifierKeys modifierKeys, Key key, Action callback)
    {
        if (_disposed) return;

        var hotKeyId = _nextHotKeyId++;
        var virtualKey = KeyInterop.VirtualKeyFromKey(key);
        var modifiers = ConvertModifierKeys(modifierKeys);

        if (RegisterHotKey(IntPtr.Zero, hotKeyId, modifiers, virtualKey))
        {
            _hotKeyIds[(modifierKeys, key)] = hotKeyId;
            _callbacks[hotKeyId] = callback;
        }
        else
        {
            throw new InvalidOperationException($"Failed to register hot key: {modifierKeys} + {key}");
        }
    }

    public void UnregisterHotKey(ModifierKeys modifierKeys, Key key)
    {
        if (_disposed) return;

        if (_hotKeyIds.TryGetValue((modifierKeys, key), out var hotKeyId))
        {
            UnregisterHotKey(IntPtr.Zero, hotKeyId);
            _hotKeyIds.Remove((modifierKeys, key));
            _callbacks.Remove(hotKeyId);
        }
    }

    public void UnregisterAllHotKeys()
    {
        if (_disposed) return;

        foreach (var hotKeyId in _hotKeyIds.Values)
        {
            UnregisterHotKey(IntPtr.Zero, hotKeyId);
        }
        _hotKeyIds.Clear();
        _callbacks.Clear();
    }

    public bool IsHotKeyRegistered(ModifierKeys modifierKeys, Key key)
    {
        return _hotKeyIds.ContainsKey((modifierKeys, key));
    }

    private void OnKeyDown(object sender, KeyEventArgs e)
    {
        // This is a simplified implementation
        // In a real implementation, you would handle WM_HOTKEY messages
        // and check if the pressed keys match any registered hot keys
    }

    private static uint ConvertModifierKeys(ModifierKeys modifierKeys)
    {
        uint modifiers = 0;
        if ((modifierKeys & ModifierKeys.Alt) == ModifierKeys.Alt)
            modifiers |= MOD_ALT;
        if ((modifierKeys & ModifierKeys.Control) == ModifierKeys.Control)
            modifiers |= MOD_CONTROL;
        if ((modifierKeys & ModifierKeys.Shift) == ModifierKeys.Shift)
            modifiers |= MOD_SHIFT;
        if ((modifierKeys & ModifierKeys.Windows) == ModifierKeys.Windows)
            modifiers |= MOD_WIN;
        return modifiers;
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            UnregisterAllHotKeys();
            _disposed = true;
        }
    }

    // Windows API declarations
    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool UnregisterHotKey(IntPtr hWnd, int id);

    // Modifier key constants
    private const uint MOD_ALT = 0x0001;
    private const uint MOD_CONTROL = 0x0002;
    private const uint MOD_SHIFT = 0x0004;
    private const uint MOD_WIN = 0x0008;
}
