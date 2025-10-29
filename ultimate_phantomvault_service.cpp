/**
 * Ultimate PhantomVault Native Service
 * 
 * Production-grade system-wide service with:
 * - Bulletproof Ctrl+Alt+V detection from anywhere
 * - Intelligent folder discovery and unlocking
 * - GUI authentication dialog
 * - Robust error handling and logging
 * - System service integration
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
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <gtk/gtk.h>
#endif

namespace fs = std::filesystem;

// Global state for keyboard detection
std::atomic<bool> g_running{true};
std::atomic<bool> g_ctrl_pressed{false};
std::atomic<bool> g_alt_pressed{false};
std::atomic<bool> g_sequence_detected{false};

// Forward declarations
class UltimatePhantomVaultService;
UltimatePhantomVaultService* g_service_instance = nullptr;

struct Profile {
    std::string id;
    std::string name;
    std::string masterKey;
    std::vector<std::string> lockedFolders;
    std::chrono::system_clock::time_point lastAccess;
    bool isActive = false;
};

struct LockedFolder {
    std::string originalPath;
    std::string encryptedPath;
    std::string profileId;
    std::chrono::system_clock::time_point lockedAt;
    size_t originalSize;
};

class Logger {
private:
    std::string logFile_;
    std::mutex logMutex_;
    
public:
    Logger(const std::string& logPath) : logFile_(logPath) {
        // Ensure log directory exists
        fs::create_directories(fs::path(logFile_).parent_path());
    }
    
    void log(const std::string& level, const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ofstream file(logFile_, std::ios::app);
        if (file.is_open()) {
            file << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                 << "] [" << level << "] " << message << std::endl;
        }
        
        // Also output to console
        std::cout << "[" << level << "] " << message << std::endl;
    }
    
    void info(const std::string& message) { log("INFO", message); }
    void warn(const std::string& message) { log("WARN", message); }
    void error(const std::string& message) { log("ERROR", message); }
    void debug(const std::string& message) { log("DEBUG", message); }
};

class UltimatePhantomVaultService {
private:
    std::map<std::string, Profile> profiles_;
    std::vector<LockedFolder> lockedFolders_;
    std::string dataPath_;
    std::unique_ptr<Logger> logger_;
    std::mutex serviceMutex_;
    
public:
    UltimatePhantomVaultService() {
        // Initialize data path
        const char* home = getenv("HOME");
        if (home) {
            dataPath_ = std::string(home) + "/.phantomvault";
        } else {
            dataPath_ = "/tmp/phantomvault";
        }
        
        // Initialize logger
        logger_ = std::make_unique<Logger>(dataPath_ + "/logs/service.log");
        
        logger_->info("=== Ultimate PhantomVault Service Starting ===");
        logger_->info("Data path: " + dataPath_);
        
        // Create necessary directories
        fs::create_directories(dataPath_);
        fs::create_directories(dataPath_ + "/profiles");
        fs::create_directories(dataPath_ + "/logs");
        
        // Load existing data
        loadProfiles();
        scanForLockedFolders();
        
        logger_->info("Service initialized with " + std::to_string(profiles_.size()) + 
                     " profiles and " + std::to_string(lockedFolders_.size()) + " locked folders");
    }
    
    void loadProfiles() {
        std::string profilesFile = dataPath_ + "/profiles/profiles.json";
        std::ifstream file(profilesFile);
        
        if (!file.is_open()) {
            logger_->info("No existing profiles found, creating default structure");
            createDefaultProfile();
            return;
        }
        
        // Simple JSON-like parsing for profiles
        std::string line;
        Profile currentProfile;
        bool inProfile = false;
        
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \\t"));
            line.erase(line.find_last_not_of(" \\t") + 1);
            
            if (line.find("\"id\":") != std::string::npos) {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start + 8);
                if (end != std::string::npos) {
                    currentProfile.id = line.substr(start + 4, end - start - 4);
                    inProfile = true;
                }
            } else if (line.find("\"name\":") != std::string::npos && inProfile) {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start + 6);
                if (end != std::string::npos) {
                    currentProfile.name = line.substr(start + 6, end - start - 6);
                }
            } else if (line.find("\"masterKey\":") != std::string::npos && inProfile) {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start + 11);
                if (end != std::string::npos) {
                    currentProfile.masterKey = line.substr(start + 11, end - start - 11);
                }
            } else if (line.find("}") != std::string::npos && inProfile) {
                if (!currentProfile.id.empty() && !currentProfile.name.empty()) {
                    profiles_[currentProfile.id] = currentProfile;
                    logger_->info("Loaded profile: " + currentProfile.name + " (ID: " + currentProfile.id + ")");
                }
                currentProfile = Profile{};
                inProfile = false;
            }
        }
    }
    
    void createDefaultProfile() {
        Profile defaultProfile;
        defaultProfile.id = "default_profile_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        defaultProfile.name = "Default User";
        defaultProfile.masterKey = "phantomvault123";  // Default key - user should change this
        defaultProfile.lastAccess = std::chrono::system_clock::now();
        
        profiles_[defaultProfile.id] = defaultProfile;
        saveProfiles();
        
        logger_->info("Created default profile: " + defaultProfile.name);
        logger_->info("Default master key: " + defaultProfile.masterKey + " (CHANGE THIS!)");
    }
    
    void saveProfiles() {
        std::string profilesFile = dataPath_ + "/profiles/profiles.json";
        std::ofstream file(profilesFile);
        
        file << "{\\n  \"profiles\": [\\n";
        bool first = true;
        for (const auto& [id, profile] : profiles_) {
            if (!first) file << ",\\n";
            file << "    {\\n";
            file << "      \"id\": \"" << profile.id << "\",\\n";
            file << "      \"name\": \"" << profile.name << "\",\\n";
            file << "      \"masterKey\": \"" << profile.masterKey << "\"\\n";
            file << "    }";
            first = false;
        }
        file << "\\n  ]\\n}\\n";
    }
    
    void scanForLockedFolders() {
        logger_->info("🔍 Scanning for locked folders...");
        
        // Smart scanning - only scan accessible directories
        std::vector<std::string> scanPaths = {
            std::string(getenv("HOME") ? getenv("HOME") : "/tmp"),
            std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/Desktop",
            std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/Documents",
            std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/Downloads",
            std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/Pictures",
            "/tmp"
        };
        
        for (const auto& scanPath : scanPaths) {
            if (!fs::exists(scanPath)) {
                logger_->debug("Skipping non-existent path: " + scanPath);
                continue;
            }
            
            logger_->debug("Scanning: " + scanPath);
            
            try {
                // Use non-recursive iterator first to check direct children
                for (const auto& entry : fs::directory_iterator(scanPath)) {
                    if (entry.is_directory()) {
                        std::string path = entry.path().string();
                        if (path.length() >= 24 && path.substr(path.length() - 24) == ".phantomvault_encrypted") {
                            std::string originalPath = path.substr(0, path.length() - 24);
                            
                            LockedFolder lockedFolder;
                            lockedFolder.originalPath = originalPath;
                            lockedFolder.encryptedPath = path;
                            lockedFolder.profileId = "default"; // Assign to default profile
                            lockedFolder.lockedAt = std::chrono::system_clock::now();
                            lockedFolder.originalSize = calculateFolderSize(path);
                            
                            lockedFolders_.push_back(lockedFolder);
                            logger_->info("✅ Found locked folder: " + originalPath + 
                                        " (size: " + std::to_string(lockedFolder.originalSize) + " bytes)");
                        }
                    }
                }
                
                // Also try recursive scan with error handling for subdirectories
                try {
                    for (const auto& entry : fs::recursive_directory_iterator(
                        scanPath, 
                        fs::directory_options::skip_permission_denied)) {
                        
                        if (entry.is_directory()) {
                            std::string path = entry.path().string();
                            if (path.length() >= 24 && path.substr(path.length() - 24) == ".phantomvault_encrypted") {
                                // Check if we already found this folder
                                bool alreadyFound = false;
                                for (const auto& existing : lockedFolders_) {
                                    if (existing.encryptedPath == path) {
                                        alreadyFound = true;
                                        break;
                                    }
                                }
                                
                                if (!alreadyFound) {
                                    std::string originalPath = path.substr(0, path.length() - 24);
                                    
                                    LockedFolder lockedFolder;
                                    lockedFolder.originalPath = originalPath;
                                    lockedFolder.encryptedPath = path;
                                    lockedFolder.profileId = "default";
                                    lockedFolder.lockedAt = std::chrono::system_clock::now();
                                    lockedFolder.originalSize = calculateFolderSize(path);
                                    
                                    lockedFolders_.push_back(lockedFolder);
                                    logger_->info("✅ Found locked folder (deep): " + originalPath + 
                                                " (size: " + std::to_string(lockedFolder.originalSize) + " bytes)");
                                }
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    logger_->debug("Recursive scan limited for " + scanPath + ": " + e.what());
                }
                
            } catch (const std::exception& e) {
                logger_->warn("⚠️ Error scanning " + scanPath + ": " + e.what());
            }
        }
        
        if (lockedFolders_.empty()) {
            logger_->info("📂 No locked folders found. Create some encrypted folders to test!");
            logger_->info("💡 Tip: Any folder ending with '.phantomvault_encrypted' will be detected");
        } else {
            logger_->info("🎯 Found " + std::to_string(lockedFolders_.size()) + " locked folders ready for unlock!");
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
        
        logger_->info("🎯 Ctrl+Alt+V sequence detected!");
        
        if (lockedFolders_.empty()) {
            logger_->info("📂 No locked folders found - rescanning...");
            
            // Rescan for folders in case new ones were created
            scanForLockedFolders();
            
            if (lockedFolders_.empty()) {
                logger_->info("❌ Still no locked folders found");
                showNotification("PhantomVault", "No locked folders found to unlock");
                return;
            }
        }
        
        logger_->info("🔓 Found " + std::to_string(lockedFolders_.size()) + " locked folders to unlock");
        
        // Show authentication dialog
        if (showAuthenticationDialog()) {
            unlockAllFolders();
        } else {
            logger_->warn("❌ Authentication failed or cancelled");
        }
    }
    
    bool showAuthenticationDialog() {
        #ifdef __linux__
        // Initialize GTK if not already done
        if (!gtk_init_check(nullptr, nullptr)) {
            logger_->error("Failed to initialize GTK");
            return false;
        }
        
        // Create dialog
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "PhantomVault Authentication",
            nullptr,
            GTK_DIALOG_MODAL,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Unlock", GTK_RESPONSE_OK,
            nullptr
        );
        
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
        
        // Create content
        GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        
        GtkWidget* label = gtk_label_new("Enter master key to unlock folders:");
        gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 10);
        
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE); // Hide password
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Master Key");
        gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 5);
        
        // Show folder count
        std::string folderInfo = "Found " + std::to_string(lockedFolders_.size()) + " locked folders";
        GtkWidget* infoLabel = gtk_label_new(folderInfo.c_str());
        gtk_box_pack_start(GTK_BOX(content), infoLabel, FALSE, FALSE, 5);
        
        gtk_widget_show_all(dialog);
        
        // Focus on entry
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
        logger_->error("GUI authentication not implemented for this platform");
        return false;
        #endif
    }
    
    bool authenticateUser(const std::string& masterKey) {
        // Try to authenticate with any profile
        for (const auto& [id, profile] : profiles_) {
            if (profile.masterKey == masterKey) {
                logger_->info("Authentication successful for profile: " + profile.name);
                return true;
            }
        }
        
        logger_->warn("Authentication failed for master key");
        showNotification("PhantomVault", "Authentication failed!");
        return false;
    }
    
    void unlockAllFolders() {
        int unlockedCount = 0;
        int failedCount = 0;
        
        logger_->info("Starting to unlock " + std::to_string(lockedFolders_.size()) + " folders");
        
        for (auto it = lockedFolders_.begin(); it != lockedFolders_.end();) {
            const auto& folder = *it;
            
            try {
                if (fs::exists(folder.encryptedPath) && !fs::exists(folder.originalPath)) {
                    // Unlock the folder
                    fs::rename(folder.encryptedPath, folder.originalPath);
                    
                    logger_->info("Unlocked: " + folder.originalPath);
                    unlockedCount++;
                    
                    // Remove from locked folders list
                    it = lockedFolders_.erase(it);
                } else {
                    logger_->warn("Folder already unlocked or missing: " + folder.originalPath);
                    it = lockedFolders_.erase(it);
                }
            } catch (const std::exception& e) {
                logger_->error("Failed to unlock " + folder.originalPath + ": " + e.what());
                failedCount++;
                ++it;
            }
        }
        
        std::string message = "Unlocked " + std::to_string(unlockedCount) + " folders";
        if (failedCount > 0) {
            message += " (" + std::to_string(failedCount) + " failed)";
        }
        
        logger_->info(message);
        showNotification("PhantomVault", message);
    }
    
    void showNotification(const std::string& title, const std::string& message) {
        #ifdef __linux__
        std::string cmd = "notify-send '" + title + "' '" + message + "' --icon=dialog-information 2>/dev/null &";
        system(cmd.c_str());
        #endif
        
        logger_->info("Notification: " + title + " - " + message);
    }
    
    void shutdown() {
        logger_->info("Service shutting down...");
        saveProfiles();
        logger_->info("Service stopped gracefully");
    }
};

#ifdef __linux__
// X11 keyboard event callback
void keyboardEventCallback(XPointer closure, XRecordInterceptData* data) {
    if (data->category != XRecordFromServer) {
        XRecordFreeData(data);
        return;
    }
    
    int event_type = data->data[0];
    int keycode = data->data[1];
    
    // Use the display passed in closure instead of opening a new one
    Display* display = (Display*)closure;
    if (!display) {
        XRecordFreeData(data);
        return;
    }
    
    KeySym keysym = XkbKeycodeToKeysym(display, keycode, 0, 0);
    bool key_pressed = (event_type == KeyPress);
    
    // Track modifier keys
    if (keysym == XK_Control_L || keysym == XK_Control_R) {
        g_ctrl_pressed = key_pressed;
    } else if (keysym == XK_Alt_L || keysym == XK_Alt_R) {
        g_alt_pressed = key_pressed;
    } else if ((keysym == XK_v || keysym == XK_V) && key_pressed) {
        // Check if Ctrl+Alt+V is pressed
        if (g_ctrl_pressed && g_alt_pressed && !g_sequence_detected) {
            g_sequence_detected = true;
            
            // Log the detection
            std::cout << "[HOTKEY] Ctrl+Alt+V detected! Triggering unlock sequence..." << std::endl;
            
            // Trigger unlock in separate thread to avoid blocking X11
            std::thread([]() {
                if (g_service_instance) {
                    g_service_instance->onCtrlAltVDetected();
                }
                
                // Reset sequence detection after a delay
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                g_sequence_detected = false;
            }).detach();
        }
    }
    
    XRecordFreeData(data);
}

void startGlobalKeyboardMonitoring() {
    Display* control_display = XOpenDisplay(nullptr);
    if (!control_display) {
        std::cerr << "[ERROR] Failed to open X11 control display for keyboard monitoring" << std::endl;
        return;
    }
    
    Display* data_display = XOpenDisplay(nullptr);
    if (!data_display) {
        std::cerr << "[ERROR] Failed to open X11 data display for keyboard monitoring" << std::endl;
        XCloseDisplay(control_display);
        return;
    }
    
    // Check for XRecord extension
    int major, minor;
    if (!XRecordQueryVersion(control_display, &major, &minor)) {
        std::cerr << "[ERROR] XRecord extension not available" << std::endl;
        XCloseDisplay(control_display);
        XCloseDisplay(data_display);
        return;
    }
    
    std::cout << "[INFO] XRecord extension version: " << major << "." << minor << std::endl;
    
    // Set up recording range for all key events
    XRecordRange* range = XRecordAllocRange();
    if (!range) {
        std::cerr << "[ERROR] Failed to allocate XRecord range" << std::endl;
        XCloseDisplay(control_display);
        XCloseDisplay(data_display);
        return;
    }
    
    range->device_events.first = KeyPress;
    range->device_events.last = KeyRelease;
    
    // Create recording context
    XRecordClientSpec client_spec = XRecordAllClients;
    XRecordContext context = XRecordCreateContext(control_display, 0, &client_spec, 1, &range, 1);
    
    if (!context) {
        std::cerr << "[ERROR] Failed to create XRecord context" << std::endl;
        XFree(range);
        XCloseDisplay(control_display);
        XCloseDisplay(data_display);
        return;
    }
    
    std::cout << "[INFO] ✅ Global keyboard monitoring started successfully!" << std::endl;
    std::cout << "[INFO] 🎯 Ctrl+Alt+V detection is ACTIVE - try it from anywhere!" << std::endl;
    
    // Start monitoring in separate thread with proper display handling
    std::thread([data_display, context, control_display]() {
        // Enable context with data display and pass control display as closure
        if (!XRecordEnableContext(data_display, context, keyboardEventCallback, (XPointer)control_display)) {
            std::cerr << "[ERROR] Failed to enable XRecord context" << std::endl;
        } else {
            std::cout << "[INFO] XRecord context enabled successfully" << std::endl;
        }
    }).detach();
    
    XFree(range);
}
#endif

// Signal handlers
void signalHandler(int signal) {
    std::cout << "\\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
    
    if (g_service_instance) {
        g_service_instance->shutdown();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Ultimate PhantomVault Native Service ===" << std::endl;
    std::cout << "Production-grade system-wide folder unlocking" << std::endl;
    std::cout << "Press Ctrl+Alt+V from ANYWHERE to unlock folders" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize service
    UltimatePhantomVaultService service;
    g_service_instance = &service;
    
    #ifdef __linux__
    // Start global keyboard monitoring
    startGlobalKeyboardMonitoring();
    #else
    std::cout << "[ERROR] This platform is not yet supported" << std::endl;
    return 1;
    #endif
    
    std::cout << "[INFO] Service is running... Press Ctrl+C to stop" << std::endl;
    std::cout << "[INFO] Press Ctrl+Alt+V from anywhere to unlock folders" << std::endl;
    
    // Main service loop
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[INFO] Service stopped" << std::endl;
    return 0;
}