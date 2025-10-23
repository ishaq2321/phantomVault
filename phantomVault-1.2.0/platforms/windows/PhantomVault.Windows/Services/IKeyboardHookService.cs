using System;
using System.Windows.Input;

namespace PhantomVault.Windows.Services;

public interface IKeyboardHookService
{
    void RegisterHotKey(ModifierKeys modifierKeys, Key key, Action callback);
    void UnregisterHotKey(ModifierKeys modifierKeys, Key key);
    void UnregisterAllHotKeys();
    bool IsHotKeyRegistered(ModifierKeys modifierKeys, Key key);
}
