/**
 * Working PhantomVault Native Service
 * 
 * Simplified but WORKING version with reliable keyboard detection
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <unistd.h>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <gtk/gtk.h>
#endif

namespace fs = std::filesystem;

// Global state
std::atomic<bool> g_running{true};
std::atomic<bool> g_sequence_detected{false};

struct LockedFolder {
    std::string originalPath;
    std::string encryptedPath;
    size_t originalSize;
};

class WorkingPhantomVaultService {
private:
    std::vector<LockedFolder> lockedFolders_;
    std::string dataPath_;
    std::mutex serviceMutex_;
    
public:
    WorkingPhantomVaultService() {
        const char* home = getenv("HOME");
        dataPath_ = home ? std::string(home) + "/.phantomvault" : "/tmp/phantomvault";
        
        std::cout << "[INFO] 🚀 Working PhantomVault Service Starting..." << std::endl;
        std::cout << "[INFO] Data path: " << dataPath_ << std::endl;
        
        fs::create_directories(dataPath_);
        scanForLockedFolders();
    }
    
    void scanForLockedFolders() {
        std::cout << "[INFO] 🔍 Scanning for locked folders..." << std::endl;
        
        lockedFolders_.clear();
        
        // Scan home directory for encrypted folders
        const char* home = getenv("HOME");
        if (!home) return;
        
        std::string homePath = home;
        
        try {
            for (const auto& entry : fs::directory_iterator(homePath)) {
                if (entry.is_directory()) {
                    std::string path = entry.path().string();
                    std::string filename = entry.path().filename().string();
                    
                    std::cout << "[DEBUG] Checking: " << filename << std::endl;
                    
                    if (filename.find(".phantomvault_encrypted") != std::string::npos) {
                        std::cout << "[DEBUG] 🎯 FOUND ENCRYPTED FOLDER: " << filename << std::endl;
                        std::string originalPath = homePath + "/" + filename.substr(0, filename.length() - 24);
                        
                        LockedFolder folder;
                        folder.originalPath = originalPath;
                        folder.encryptedPath = path;
                        folder.originalSize = calculateFolderSize(path);
                        
                        lockedFolders_.push_back(folder);
                        std::cout << "[INFO] ✅ Found locked folder: " << originalPath 
                                  << " (size: " << folder.originalSize << " bytes)" << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[WARN] Error scanning home directory: " << e.what() << std::endl;
        }
        
        if (lockedFolders_.empty()) {
            std::cout << "[INFO] 📂 No locked folders found" << std::endl;
        } else {
            std::cout << "[INFO] 🎯 Found " << lockedFolders_.size() << " locked folders!" << std::endl;
        }
    }
    
    size_t calculateFolderSize(const std::string& path) {
        size_t size = 0;
        try {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    size += entry.file_size();
                }
            }
        } catch (const std::exception&) {
            // Ignore errors
        }
        return size;
    }
    
    void onCtrlAltVDetected() {
        std::lock_guard<std::mutex> lock(serviceMutex_);
        
        std::cout << "[HOTKEY] 🎯 Ctrl+Alt+V detected! Processing..." << std::endl;
        
        // Rescan for new folders
        scanForLockedFolders();
        
        if (lockedFolders_.empty()) {
            std::cout << "[INFO] ❌ No locked folders found to unlock" << std::endl;
            showNotification("PhantomVault", "No locked folders found");
            return;
        }
        
        std::cout << "[INFO] 🔓 Found " << lockedFolders_.size() << " folders to unlock" << std::endl;
        
        // Show authentication dialog
        if (showAuthenticationDialog()) {
            unlockAllFolders();
        }
    }
    
    bool showAuthenticationDialog() {
        #ifdef __linux__
        std::cout << "[AUTH] 🔐 Showing authentication dialog..." << std::endl;
        
        // Initialize GTK if not already done
        if (!gtk_init_check(nullptr, nullptr)) {
            std::cout << "[ERROR] Failed to initialize GTK" << std::endl;
            return false;
        }
        
        // Create dialog
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "PhantomVault - Unlock Folders",
            nullptr,
            GTK_DIALOG_MODAL,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Unlock", GTK_RESPONSE_OK,
            nullptr
        );
        
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
        
        // Create content
        GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(content), 20);
        
        GtkWidget* label = gtk_label_new("🔐 Enter master key to unlock encrypted folders:");
        gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 10);
        
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE); // Hide password
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Master Key");
        gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 10);
        
        // Show folder count
        std::string folderInfo = "🎯 " + std::to_string(lockedFolders_.size()) + " encrypted folders found";
        GtkWidget* infoLabel = gtk_label_new(folderInfo.c_str());
        gtk_box_pack_start(GTK_BOX(content), infoLabel, FALSE, FALSE, 10);
        
        gtk_widget_show_all(dialog);
        gtk_widget_grab_focus(entry);
        
        // Run dialog
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        
        bool authenticated = false;
        if (response == GTK_RESPONSE_OK) {
            const char* password = gtk_entry_get_text(GTK_ENTRY(entry));
            authenticated = authenticateUser(std::string(password));
        }
        
        gtk_widget_destroy(dialog);
        
        // Process pending GTK events
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
        
        return authenticated;
        #else
        return false;
        #endif
    }
    
    bool authenticateUser(const std::string& masterKey) {
        // Simple authentication - accept "phantomvault123" or "test123"
        if (masterKey == "phantomvault123" || masterKey == "test123") {
            std::cout << "[AUTH] ✅ Authentication successful!" << std::endl;
            return true;
        }
        
        std::cout << "[AUTH] ❌ Authentication failed" << std::endl;
        showNotification("PhantomVault", "Authentication failed!");
        return false;
    }
    
    void unlockAllFolders() {
        int unlockedCount = 0;
        
        std::cout << "[UNLOCK] 🔓 Starting to unlock " << lockedFolders_.size() << " folders..." << std::endl;
        
        for (const auto& folder : lockedFolders_) {
            try {
                if (fs::exists(folder.encryptedPath) && !fs::exists(folder.originalPath)) {
                    fs::rename(folder.encryptedPath, folder.originalPath);
                    std::cout << "[UNLOCK] ✅ Unlocked: " << folder.originalPath << std::endl;
                    unlockedCount++;
                } else {
                    std::cout << "[UNLOCK] ⚠️ Folder already unlocked: " << folder.originalPath << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "[UNLOCK] ❌ Failed to unlock " << folder.originalPath << ": " << e.what() << std::endl;
            }
        }
        
        std::string message = "Unlocked " + std::to_string(unlockedCount) + " folders successfully!";
        std::cout << "[UNLOCK] 🎉 " << message << std::endl;
        showNotification("PhantomVault", message);
        
        // Clear the list since folders are now unlocked
        lockedFolders_.clear();
    }
    
    void showNotification(const std::string& title, const std::string& message) {
        #ifdef __linux__
        std::string cmd = "notify-send '" + title + "' '" + message + "' --icon=dialog-information 2>/dev/null &";
        system(cmd.c_str());
        #endif
    }
};

WorkingPhantomVaultService* g_service_instance = nullptr;

#ifdef __linux__
// Simple X11 keyboard polling (more reliable than XRecord)
void keyboardPollingThread() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cout << "[ERROR] Cannot open X11 display for keyboard monitoring" << std::endl;
        return;
    }
    
    std::cout << "[INFO] ✅ Keyboard polling started - monitoring Ctrl+Alt+V" << std::endl;
    
    bool ctrl_pressed = false;
    bool alt_pressed = false;
    
    while (g_running) {
        // Query keyboard state
        char keys[32];
        XQueryKeymap(display, keys);
        
        // Check modifier keys
        KeyCode ctrl_keycode = XKeysymToKeycode(display, XK_Control_L);
        KeyCode ctrl_r_keycode = XKeysymToKeycode(display, XK_Control_R);
        KeyCode alt_keycode = XKeysymToKeycode(display, XK_Alt_L);
        KeyCode alt_r_keycode = XKeysymToKeycode(display, XK_Alt_R);
        KeyCode v_keycode = XKeysymToKeycode(display, XK_v);
        
        bool ctrl_now = (keys[ctrl_keycode / 8] & (1 << (ctrl_keycode % 8))) || 
                       (keys[ctrl_r_keycode / 8] & (1 << (ctrl_r_keycode % 8)));
        bool alt_now = (keys[alt_keycode / 8] & (1 << (alt_keycode % 8))) || 
                      (keys[alt_r_keycode / 8] & (1 << (alt_r_keycode % 8)));
        bool v_now = (keys[v_keycode / 8] & (1 << (v_keycode % 8)));
        
        // Detect Ctrl+Alt+V combination
        if (ctrl_now && alt_now && v_now && !g_sequence_detected) {
            g_sequence_detected = true;
            
            std::cout << "[HOTKEY] 🎯 Ctrl+Alt+V detected!" << std::endl;
            
            if (g_service_instance) {
                std::thread([](){ 
                    g_service_instance->onCtrlAltVDetected();
                    
                    // Reset detection after delay
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                    g_sequence_detected = false;
                }).detach();
            }
        }
        
        // Small delay to avoid excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    XCloseDisplay(display);
}
#endif

void signalHandler(int signal) {
    std::cout << "\\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

int main() {
    std::cout << "=== Working PhantomVault Native Service ===" << std::endl;
    std::cout << "Reliable system-wide Ctrl+Alt+V detection" << std::endl;
    std::cout << "Press Ctrl+Alt+V from ANYWHERE to unlock folders" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    WorkingPhantomVaultService service;
    g_service_instance = &service;
    
    #ifdef __linux__
    // Start keyboard monitoring thread
    std::thread keyboard_thread(keyboardPollingThread);
    #endif
    
    std::cout << "[INFO] 🚀 Service is running and monitoring keyboard..." << std::endl;
    std::cout << "[INFO] 🎯 Press Ctrl+Alt+V from anywhere to test!" << std::endl;
    std::cout << "[INFO] Default master keys: 'phantomvault123' or 'test123'" << std::endl;
    
    // Main loop
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    #ifdef __linux__
    if (keyboard_thread.joinable()) {
        keyboard_thread.join();
    }
    #endif
    
    std::cout << "[INFO] Service stopped" << std::endl;
    return 0;
}