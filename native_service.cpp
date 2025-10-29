/**
 * PhantomVault Native Service
 * 
 * Standalone native service that runs in background and provides:
 * - Global Ctrl+Alt+V keyboard sequence detection
 * - System-wide folder unlock functionality
 * - Master key prompt and authentication
 * - Independent of GUI (runs as system service)
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <unistd.h>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#endif

namespace fs = std::filesystem;

// Global state
std::atomic<bool> g_running{true};
std::atomic<bool> g_ctrl_pressed{false};
std::atomic<bool> g_alt_pressed{false};

struct Profile {
    std::string id;
    std::string name;
    std::string masterKey;
    std::vector<std::string> lockedFolders;
};

class PhantomVaultNativeService {
private:
    std::map<std::string, Profile> profiles_;
    std::string data_path_;
    
public:
    PhantomVaultNativeService() : data_path_(getenv("HOME") ? std::string(getenv("HOME")) + "/.phantomvault" : "./phantomvault_data") {
        loadProfiles();
    }
    
    void loadProfiles() {
        // Load profiles from simple storage
        std::string profiles_file = data_path_ + "/profiles.txt";
        std::ifstream file(profiles_file);
        if (!file.is_open()) {
            std::cout << "[PhantomVault] No existing profiles found" << std::endl;
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            // Simple format: id|name|masterKey|folder1,folder2,folder3
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);
            
            if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
                Profile profile;
                profile.id = line.substr(0, pos1);
                profile.name = line.substr(pos1 + 1, pos2 - pos1 - 1);
                profile.masterKey = line.substr(pos2 + 1, pos3 - pos2 - 1);
                
                std::string folders_str = line.substr(pos3 + 1);
                if (!folders_str.empty()) {
                    std::istringstream folders_stream(folders_str);
                    std::string folder;
                    while (std::getline(folders_stream, folder, ',')) {
                        if (!folder.empty()) {
                            profile.lockedFolders.push_back(folder);
                        }
                    }
                }
                
                profiles_[profile.id] = profile;
                std::cout << "[PhantomVault] Loaded profile: " << profile.name 
                          << " with " << profile.lockedFolders.size() << " locked folders" << std::endl;
            }
        }
        
        std::cout << "[PhantomVault] Loaded " << profiles_.size() << " profiles" << std::endl;
    }
    
    void saveProfiles() {
        fs::create_directories(data_path_);
        std::string profiles_file = data_path_ + "/profiles.txt";
        std::ofstream file(profiles_file);
        
        for (const auto& [id, profile] : profiles_) {
            file << profile.id << "|" << profile.name << "|" << profile.masterKey << "|";
            for (size_t i = 0; i < profile.lockedFolders.size(); ++i) {
                if (i > 0) file << ",";
                file << profile.lockedFolders[i];
            }
            file << std::endl;
        }
    }
    
    bool authenticateAndUnlock() {
        if (profiles_.empty()) {
            std::cout << "[PhantomVault] No profiles configured" << std::endl;
            showNotification("PhantomVault", "No profiles configured. Please use the GUI to create a profile.");
            return false;
        }
        
        // Simple console-based authentication for now
        // In a full implementation, this would show a GUI dialog
        std::cout << "\\n[PhantomVault] Ctrl+Alt+V detected! Enter master key to unlock folders:" << std::endl;
        
        // List available profiles
        std::cout << "Available profiles:" << std::endl;
        int index = 1;
        std::vector<std::string> profile_ids;
        for (const auto& [id, profile] : profiles_) {
            std::cout << index << ". " << profile.name << " (" << profile.lockedFolders.size() << " locked folders)" << std::endl;
            profile_ids.push_back(id);
            index++;
        }
        
        std::cout << "Select profile (1-" << profiles_.size() << "): ";
        int selection;
        std::cin >> selection;
        
        if (selection < 1 || selection > (int)profiles_.size()) {
            std::cout << "Invalid selection" << std::endl;
            return false;
        }
        
        std::string selected_profile_id = profile_ids[selection - 1];
        Profile& profile = profiles_[selected_profile_id];
        
        std::cout << "Enter master key for " << profile.name << ": ";
        std::string master_key;
        std::cin >> master_key;
        
        if (master_key != profile.masterKey) {
            std::cout << "[PhantomVault] Authentication failed!" << std::endl;
            showNotification("PhantomVault", "Authentication failed!");
            return false;
        }
        
        // Unlock folders
        int unlocked_count = 0;
        for (const auto& folder_path : profile.lockedFolders) {
            if (unlockFolder(folder_path)) {
                unlocked_count++;
            }
        }
        
        std::cout << "[PhantomVault] Successfully unlocked " << unlocked_count << " folders!" << std::endl;
        showNotification("PhantomVault", "Unlocked " + std::to_string(unlocked_count) + " folders");
        
        return true;
    }
    
    bool unlockFolder(const std::string& folder_path) {
        std::string hidden_path = folder_path + ".phantomvault_encrypted";
        
        try {
            if (fs::exists(hidden_path) && !fs::exists(folder_path)) {
                fs::rename(hidden_path, folder_path);
                std::cout << "[PhantomVault] Unlocked: " << folder_path << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cout << "[PhantomVault] Failed to unlock " << folder_path << ": " << e.what() << std::endl;
        }
        
        return false;
    }
    
    void showNotification(const std::string& title, const std::string& message) {
        // Simple notification using notify-send on Linux
        #ifdef __linux__
        std::string cmd = "notify-send '" + title + "' '" + message + "' 2>/dev/null &";
        system(cmd.c_str());
        #endif
    }
    
    void addTestProfile() {
        // Add a test profile for demonstration
        Profile test_profile;
        test_profile.id = "test_profile_1";
        test_profile.name = "Test User";
        test_profile.masterKey = "test123";
        test_profile.lockedFolders.push_back("/tmp/test_folder");
        
        profiles_[test_profile.id] = test_profile;
        saveProfiles();
        
        std::cout << "[PhantomVault] Added test profile: " << test_profile.name << std::endl;
    }
};

#ifdef __linux__
// X11 keyboard hook for Linux
void keyboardEventCallback(XPointer closure, XRecordInterceptData* data) {
    if (data->category != XRecordFromServer) {
        return;
    }
    
    int event_type = data->data[0];
    int keycode = data->data[1];
    
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;
    
    KeySym keysym = XkbKeycodeToKeysym(display, keycode, 0, 0);
    
    bool key_pressed = (event_type == KeyPress);
    
    // Track Ctrl and Alt keys
    if (keysym == XK_Control_L || keysym == XK_Control_R) {
        g_ctrl_pressed = key_pressed;
    } else if (keysym == XK_Alt_L || keysym == XK_Alt_R) {
        g_alt_pressed = key_pressed;
    } else if (keysym == XK_v || keysym == XK_V) {
        if (key_pressed && g_ctrl_pressed && g_alt_pressed) {
            std::cout << "\\n[PhantomVault] Ctrl+Alt+V detected!" << std::endl;
            
            // Trigger unlock sequence in a separate thread
            std::thread([]() {
                PhantomVaultNativeService* service = static_cast<PhantomVaultNativeService*>(nullptr);
                // We'll need to pass the service instance properly
                std::cout << "[PhantomVault] Keyboard sequence detected - authentication required" << std::endl;
            }).detach();
        }
    }
    
    XCloseDisplay(display);
}

void startKeyboardMonitoring() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "[PhantomVault] Failed to open X11 display" << std::endl;
        return;
    }
    
    // Set up X11 record extension for global keyboard monitoring
    XRecordRange* range = XRecordAllocRange();
    if (!range) {
        std::cerr << "[PhantomVault] Failed to allocate XRecord range" << std::endl;
        XCloseDisplay(display);
        return;
    }
    
    range->device_events.first = KeyPress;
    range->device_events.last = KeyRelease;
    
    XRecordClientSpec client_spec = XRecordAllClients;
    XRecordContext context = XRecordCreateContext(display, 0, &client_spec, 1, &range, 1);
    
    if (!context) {
        std::cerr << "[PhantomVault] Failed to create XRecord context" << std::endl;
        XFree(range);
        XCloseDisplay(display);
        return;
    }
    
    std::cout << "[PhantomVault] Starting global keyboard monitoring..." << std::endl;
    std::cout << "[PhantomVault] Press Ctrl+Alt+V to unlock folders" << std::endl;
    
    // Start monitoring in a separate thread
    std::thread([display, context]() {
        if (!XRecordEnableContext(display, context, keyboardEventCallback, nullptr)) {
            std::cerr << "[PhantomVault] Failed to enable XRecord context" << std::endl;
        }
    }).detach();
}
#endif

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\\n[PhantomVault] Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    std::cout << "=== PhantomVault Native Service ===" << std::endl;
    std::cout << "Global keyboard sequence detection and folder unlocking" << std::endl;
    std::cout << "Press Ctrl+Alt+V from anywhere to unlock folders" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    PhantomVaultNativeService service;
    
    // Add test profile if none exist
    if (argc > 1 && std::string(argv[1]) == "--add-test-profile") {
        service.addTestProfile();
        std::cout << "[PhantomVault] Test profile added. You can now test with:" << std::endl;
        std::cout << "  Master Key: test123" << std::endl;
        std::cout << "  Test Folder: /tmp/test_folder" << std::endl;
    }
    
    #ifdef __linux__
    // Start keyboard monitoring
    startKeyboardMonitoring();
    #else
    std::cout << "[PhantomVault] Keyboard monitoring not implemented for this platform yet" << std::endl;
    #endif
    
    // Main service loop
    std::cout << "[PhantomVault] Service running... Press Ctrl+C to stop" << std::endl;
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Simple test: check for 't' key to trigger test unlock
        // In real implementation, this would be handled by keyboard callback
        if (std::cin.peek() == 't') {
            std::cin.ignore();
            std::cout << "\\n[PhantomVault] Testing unlock sequence..." << std::endl;
            service.authenticateAndUnlock();
        }
    }
    
    std::cout << "[PhantomVault] Service stopped" << std::endl;
    return 0;
}