#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace phantom_vault::biometric {

/**
 * @brief Biometric authentication types
 */
enum class PHANTOM_VAULT_EXPORT BiometricType {
    Fingerprint,
    Face,
    Iris,
    Voice,
    Palm,
    Retina
};

/**
 * @brief Biometric authentication status
 */
enum class PHANTOM_VAULT_EXPORT BiometricStatus {
    Available,      // Biometric sensor is available
    Unavailable,    // Biometric sensor is not available
    NotEnrolled,    // No biometric data enrolled
    Enrolled,       // Biometric data is enrolled
    Error,          // Error occurred
    Timeout,        // Authentication timeout
    Cancelled,      // User cancelled authentication
    Failed          // Authentication failed
};

/**
 * @brief Biometric authentication result
 */
struct PHANTOM_VAULT_EXPORT BiometricResult {
    bool success;                           // Authentication success
    BiometricStatus status;                 // Authentication status
    std::string errorMessage;               // Error message if failed
    std::string biometricId;                // Biometric identifier
    std::chrono::system_clock::time_point timestamp; // Authentication timestamp
    float confidence;                       // Confidence score (0.0 - 1.0)
    std::string deviceId;                   // Device identifier
};

/**
 * @brief Biometric enrollment data
 */
struct PHANTOM_VAULT_EXPORT BiometricEnrollment {
    std::string userId;                     // User identifier
    BiometricType type;                     // Biometric type
    std::string biometricId;                // Unique biometric identifier
    std::vector<uint8_t> templateData;      // Encrypted biometric template
    std::vector<uint8_t> metadata;          // Additional metadata
    std::chrono::system_clock::time_point enrolledTime;
    std::chrono::system_clock::time_point lastUsed;
    bool isActive;                          // Enrollment status
    int quality;                            // Template quality score
};

/**
 * @brief Biometric device information
 */
struct PHANTOM_VAULT_EXPORT BiometricDevice {
    std::string deviceId;                   // Device identifier
    std::string deviceName;                 // Device name
    BiometricType type;                     // Supported biometric type
    std::string manufacturer;               // Device manufacturer
    std::string model;                      // Device model
    std::string version;                    // Driver version
    bool isAvailable;                       // Device availability
    int maxEnrollments;                     // Maximum enrollments
    float accuracy;                         // Device accuracy
};

/**
 * @brief Biometric authentication interface
 */
class PHANTOM_VAULT_EXPORT BiometricAuthenticator {
public:
    virtual ~BiometricAuthenticator() = default;
    
    // Device management
    virtual std::vector<BiometricDevice> getAvailableDevices() = 0;
    virtual bool isDeviceAvailable(BiometricType type) = 0;
    virtual BiometricStatus getDeviceStatus(BiometricType type) = 0;
    
    // Enrollment
    virtual bool startEnrollment(const std::string& userId, BiometricType type) = 0;
    virtual BiometricResult processEnrollment(const std::vector<uint8_t>& biometricData) = 0;
    virtual bool completeEnrollment(const std::string& biometricId) = 0;
    virtual bool cancelEnrollment() = 0;
    
    // Authentication
    virtual bool startAuthentication(BiometricType type) = 0;
    virtual BiometricResult processAuthentication(const std::vector<uint8_t>& biometricData) = 0;
    virtual bool cancelAuthentication() = 0;
    
    // Enrollment management
    virtual std::vector<BiometricEnrollment> getUserEnrollments(const std::string& userId) = 0;
    virtual bool deleteEnrollment(const std::string& biometricId) = 0;
    virtual bool updateEnrollment(const BiometricEnrollment& enrollment) = 0;
    
    // Event callbacks
    virtual void setEnrollmentProgressCallback(std::function<void(int)> callback) = 0;
    virtual void setAuthenticationCallback(std::function<void(const BiometricResult&)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

/**
 * @brief Windows Hello biometric authenticator
 */
class PHANTOM_VAULT_EXPORT WindowsHelloAuthenticator : public BiometricAuthenticator {
public:
    WindowsHelloAuthenticator();
    ~WindowsHelloAuthenticator() override;
    
    // Device management
    std::vector<BiometricDevice> getAvailableDevices() override;
    bool isDeviceAvailable(BiometricType type) override;
    BiometricStatus getDeviceStatus(BiometricType type) override;
    
    // Enrollment
    bool startEnrollment(const std::string& userId, BiometricType type) override;
    BiometricResult processEnrollment(const std::vector<uint8_t>& biometricData) override;
    bool completeEnrollment(const std::string& biometricId) override;
    bool cancelEnrollment() override;
    
    // Authentication
    bool startAuthentication(BiometricType type) override;
    BiometricResult processAuthentication(const std::vector<uint8_t>& biometricData) override;
    bool cancelAuthentication() override;
    
    // Enrollment management
    std::vector<BiometricEnrollment> getUserEnrollments(const std::string& userId) override;
    bool deleteEnrollment(const std::string& biometricId) override;
    bool updateEnrollment(const BiometricEnrollment& enrollment) override;
    
    // Event callbacks
    void setEnrollmentProgressCallback(std::function<void(int)> callback) override;
    void setAuthenticationCallback(std::function<void(const BiometricResult&)> callback) override;
    void setErrorCallback(std::function<void(const std::string&)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Linux biometric authenticator (using libfprint)
 */
class PHANTOM_VAULT_EXPORT LinuxBiometricAuthenticator : public BiometricAuthenticator {
public:
    LinuxBiometricAuthenticator();
    ~LinuxBiometricAuthenticator() override;
    
    // Device management
    std::vector<BiometricDevice> getAvailableDevices() override;
    bool isDeviceAvailable(BiometricType type) override;
    BiometricStatus getDeviceStatus(BiometricType type) override;
    
    // Enrollment
    bool startEnrollment(const std::string& userId, BiometricType type) override;
    BiometricResult processEnrollment(const std::vector<uint8_t>& biometricData) override;
    bool completeEnrollment(const std::string& biometricId) override;
    bool cancelEnrollment() override;
    
    // Authentication
    bool startAuthentication(BiometricType type) override;
    BiometricResult processAuthentication(const std::vector<uint8_t>& biometricData) override;
    bool cancelAuthentication() override;
    
    // Enrollment management
    std::vector<BiometricEnrollment> getUserEnrollments(const std::string& userId) override;
    bool deleteEnrollment(const std::string& biometricId) override;
    bool updateEnrollment(const BiometricEnrollment& enrollment) override;
    
    // Event callbacks
    void setEnrollmentProgressCallback(std::function<void(int)> callback) override;
    void setAuthenticationCallback(std::function<void(const BiometricResult&)> callback) override;
    void setErrorCallback(std::function<void(const std::string&)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Biometric manager for cross-platform support
 */
class PHANTOM_VAULT_EXPORT BiometricManager {
public:
    BiometricManager();
    ~BiometricManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Device management
    std::vector<BiometricDevice> getAvailableDevices();
    bool isBiometricAvailable();
    BiometricStatus getBiometricStatus();
    
    // Authentication
    bool authenticateUser(const std::string& userId, BiometricType preferredType = BiometricType::Fingerprint);
    BiometricResult getLastAuthenticationResult();
    
    // Enrollment
    bool enrollUser(const std::string& userId, BiometricType type);
    bool isUserEnrolled(const std::string& userId, BiometricType type);
    bool removeUserEnrollment(const std::string& userId, BiometricType type);
    
    // Configuration
    void setAuthenticationTimeout(std::chrono::seconds timeout);
    void setRequiredConfidence(float confidence);
    void setMaxRetryAttempts(int attempts);
    
    // Event callbacks
    void setAuthenticationSuccessCallback(std::function<void(const std::string&)> callback);
    void setAuthenticationFailureCallback(std::function<void(const std::string&)> callback);
    void setEnrollmentCompleteCallback(std::function<void(const std::string&, BiometricType)> callback);
    void setErrorCallback(std::function<void(const std::string&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Biometric security utilities
 */
class PHANTOM_VAULT_EXPORT BiometricSecurity {
public:
    // Template encryption
    static std::vector<uint8_t> encryptTemplate(const std::vector<uint8_t>& templateData, const std::string& key);
    static std::vector<uint8_t> decryptTemplate(const std::vector<uint8_t>& encryptedData, const std::string& key);
    
    // Template hashing
    static std::string hashTemplate(const std::vector<uint8_t>& templateData);
    static bool verifyTemplateHash(const std::vector<uint8_t>& templateData, const std::string& hash);
    
    // Secure storage
    static bool storeTemplateSecurely(const std::string& userId, const std::string& biometricId, 
                                     const std::vector<uint8_t>& templateData);
    static std::vector<uint8_t> retrieveTemplateSecurely(const std::string& userId, const std::string& biometricId);
    static bool deleteTemplateSecurely(const std::string& userId, const std::string& biometricId);
    
    // Quality assessment
    static int assessTemplateQuality(const std::vector<uint8_t>& templateData);
    static bool isTemplateValid(const std::vector<uint8_t>& templateData);
    
    // Anti-spoofing
    static bool detectSpoofing(const std::vector<uint8_t>& biometricData);
    static float calculateLivenessScore(const std::vector<uint8_t>& biometricData);
};

} // namespace phantom_vault::biometric
