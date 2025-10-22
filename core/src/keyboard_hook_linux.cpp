#include "phantom_vault/keyboard_hook.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <sstream>
#include <cstring>

namespace phantom_vault {

class KeyboardHook::Implementation {
public:
    Implementation() 
        : display_(nullptr)
        , root_(0)
        , is_monitoring_(false)
        , should_stop_(false)
        , monitor_thread_()
        , callback_()
        , last_error_()
    {}

    ~Implementation() {
        stopMonitoring();
        if (display_) {
            XCloseDisplay(display_);
        }
    }

    bool initialize() {
        // Open X display
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            last_error_ = "Failed to open X display";
            return false;
        }

        // Check for XInput2 extension
        int event_base, error_base;
        int major = 2, minor = 0;
        if (!XQueryExtension(display_, "XInputExtension", &xi_opcode_, &event_base, &error_base) ||
            XIQueryVersion(display_, &major, &minor) != Success) {
            last_error_ = "XInput2 extension not available";
            XCloseDisplay(display_);
            display_ = nullptr;
            return false;
        }

        root_ = DefaultRootWindow(display_);
        return true;
    }

    bool startMonitoring(KeyCallback callback) {
        if (!display_) {
            last_error_ = "Not initialized";
            return false;
        }

        if (is_monitoring_) {
            last_error_ = "Already monitoring";
            return false;
        }

        callback_ = std::move(callback);

        // Set up event mask for keyboard events
        XIEventMask mask;
        unsigned char mask_bytes[XIMaskLen(XI_LASTEVENT)] = {0};
        mask.deviceid = XIAllDevices;
        mask.mask_len = sizeof(mask_bytes);
        mask.mask = mask_bytes;
        XISetMask(mask.mask, XI_KeyPress);
        XISetMask(mask.mask, XI_KeyRelease);

        // Select events on root window
        if (XISelectEvents(display_, root_, &mask, 1) != Success) {
            last_error_ = "Failed to select events";
            return false;
        }

        XSync(display_, false);

        // Start monitoring thread
        should_stop_ = false;
        is_monitoring_ = true;
        monitor_thread_ = std::thread(&Implementation::monitorLoop, this);

        return true;
    }

    void stopMonitoring() {
        if (is_monitoring_) {
            should_stop_ = true;
            if (monitor_thread_.joinable()) {
                monitor_thread_.join();
            }
            is_monitoring_ = false;
        }
    }

    bool isMonitoring() const {
        return is_monitoring_;
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    void monitorLoop() {
        XEvent event;
        XGenericEventCookie *cookie;

        while (!should_stop_) {
            // Wait for events with timeout
            if (!XPending(display_)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            XNextEvent(display_, &event);
            cookie = &event.xcookie;

            if (cookie->type != GenericEvent || cookie->extension != xi_opcode_ || 
                !XGetEventData(display_, cookie)) {
                continue;
            }

            if (cookie->evtype == XI_KeyPress || cookie->evtype == XI_KeyRelease) {
                processKeyEvent(cookie);
            }

            XFreeEventData(display_, cookie);
        }
    }

    void processKeyEvent(XGenericEventCookie* cookie) {
        XIDeviceEvent* event = static_cast<XIDeviceEvent*>(cookie->data);
        
        // Get key name
        KeySym keysym = XkbKeycodeToKeysym(display_, event->detail, 0, 0);
        if (keysym == NoSymbol) {
            return;
        }

        char* key_name = XKeysymToString(keysym);
        if (!key_name) {
            return;
        }

        // Get modifiers
        unsigned int modifiers = 0;
        if (event->mods.effective & ShiftMask) modifiers |= ShiftMask;
        if (event->mods.effective & ControlMask) modifiers |= ControlMask;
        if (event->mods.effective & Mod1Mask) modifiers |= Mod1Mask;    // Alt
        if (event->mods.effective & Mod4Mask) modifiers |= Mod4Mask;    // Super/Windows

        // Call the callback
        callback_(key_name, cookie->evtype == XI_KeyPress, modifiers);
    }

    Display* display_;
    Window root_;
    int xi_opcode_;
    std::atomic<bool> is_monitoring_;
    std::atomic<bool> should_stop_;
    std::thread monitor_thread_;
    KeyCallback callback_;
    std::string last_error_;
};

// Public interface implementation
KeyboardHook::KeyboardHook() : pimpl(std::make_unique<Implementation>()) {}
KeyboardHook::~KeyboardHook() = default;

bool KeyboardHook::initialize() {
    return pimpl->initialize();
}

bool KeyboardHook::startMonitoring(KeyCallback callback) {
    return pimpl->startMonitoring(std::move(callback));
}

void KeyboardHook::stopMonitoring() {
    pimpl->stopMonitoring();
}

bool KeyboardHook::isMonitoring() const {
    return pimpl->isMonitoring();
}

std::string KeyboardHook::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantom_vault 