#include "phantom_vault/storage.hpp"
#include "phantom_vault/encryption.hpp"
#include "phantom_vault/filesystem.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

using json = nlohmann::json;
using namespace std::chrono;

namespace phantom_vault {
namespace storage {

namespace {
    constexpr size_t SALT_SIZE = 32;
    constexpr uint32_t DEFAULT_ITERATIONS = 100000;
    constexpr uint32_t MAX_RECOVERY_ATTEMPTS = 3;
    const std::string METADATA_DIR = ".phantom_vault";
    const std::string CONFIG_DIR = "config";
    const std::string RECOVERY_DIR = "recovery";
    
    // Simple base64 encoding for binary data
    std::string base64_encode(const std::vector<uint8_t>& data) {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int val = 0, valb = -6;
        for (uint8_t c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (result.size() % 4) result.push_back('=');
        return result;
    }
    
    std::vector<uint8_t> base64_decode(const std::string& encoded) {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::vector<uint8_t> result;
        int val = 0, valb = -8;
        for (char c : encoded) {
            if (chars.find(c) == std::string::npos) break;
            val = (val << 6) + chars.find(c);
            valb += 6;
            if (valb >= 0) {
                result.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return result;
    }
    
    std::string generateUUID() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        ss << std::hex;
        
        for (int i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (int i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (int i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << std::hex << (dis(gen) & 0x3 | 0x8);
        for (int i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (int i = 0; i < 12; i++) {
            ss << dis(gen);
        }
        
        return ss.str();
    }

    std::vector<uint8_t> generateSalt() {
        std::vector<uint8_t> salt(SALT_SIZE);
        std::random_device rd;
        std::generate(salt.begin(), salt.end(), std::ref(rd));
        return salt;
    }
}

class SecureStorage::Implementation {
public:
    Implementation() : encryption_(), fs_() {
        encryption_.initialize();
        fs_.createDirectories(METADATA_DIR);
        fs_.createDirectories(METADATA_DIR + "/" + CONFIG_DIR);
        fs_.createDirectories(METADATA_DIR + "/" + RECOVERY_DIR);
    }

    bool initialize(const std::vector<uint8_t>& master_key) {
        key_ = master_key;
        iv_ = encryption_.generateIV();
        return true;
    }

    bool saveVaultMetadata(const VaultMetadata& metadata) {
        json j;
        j["vault_id"] = metadata.vault_id;
        j["name"] = metadata.name;
        j["description"] = metadata.description;
        j["location"] = metadata.location.string();
        j["created_time"] = system_clock::to_time_t(metadata.created_time);
        j["modified_time"] = system_clock::to_time_t(metadata.modified_time);
        j["key_verification"] = metadata.key_verification;
        j["salt"] = metadata.salt;
        j["iterations"] = metadata.iterations;

        std::string json_str = j.dump();
        std::vector<uint8_t> data(json_str.begin(), json_str.end());
        
        std::vector<uint8_t> encrypted = encryption_.encryptData(data, key_, iv_);
        if (encrypted.empty()) {
            last_error_ = "Encryption failed";
            return false;
        }

        std::string filepath = METADATA_DIR + "/" + metadata.vault_id + ".meta";
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            last_error_ = "Failed to open metadata file";
            return false;
        }

        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return true;
    }

    std::optional<VaultMetadata> loadVaultMetadata(const std::string& vault_id) {
        std::string filepath = METADATA_DIR + "/" + vault_id + ".meta";
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            last_error_ = "Failed to open metadata file";
            return std::nullopt;
        }

        auto size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> encrypted(size);
        file.read(reinterpret_cast<char*>(encrypted.data()), size);

        std::vector<uint8_t> decrypted = encryption_.decryptData(encrypted, key_, iv_);
        if (decrypted.empty()) {
            last_error_ = "Decryption failed";
            return std::nullopt;
        }

        std::string json_str(decrypted.begin(), decrypted.end());
        json j = json::parse(json_str);

        VaultMetadata metadata;
        metadata.vault_id = j["vault_id"];
        metadata.name = j["name"];
        metadata.description = j["description"];
        metadata.location = j["location"].get<std::string>();
        metadata.created_time = system_clock::from_time_t(j["created_time"]);
        metadata.modified_time = system_clock::from_time_t(j["modified_time"]);
        metadata.key_verification = j["key_verification"].get<std::vector<uint8_t>>();
        metadata.salt = j["salt"].get<std::vector<uint8_t>>();
        metadata.iterations = j["iterations"];

        return metadata;
    }

    std::vector<std::string> listVaults() {
        std::vector<std::string> vaults;
        for (const auto& entry : std::filesystem::directory_iterator(METADATA_DIR)) {
            if (entry.path().extension() == ".meta") {
                vaults.push_back(entry.path().stem().string());
            }
        }
        return vaults;
    }

    bool deleteVaultMetadata(const std::string& vault_id) {
        std::string filepath = METADATA_DIR + "/" + vault_id + ".meta";
        return fs_.remove(filepath, false);
    }

    bool saveVaultConfig(const std::string& vault_id, const VaultConfig& config) {
        json j;
        j["auto_lock"] = config.auto_lock;
        j["lock_timeout"] = config.lock_timeout.count();
        j["clear_clipboard"] = config.clear_clipboard;
        j["clipboard_timeout"] = config.clipboard_timeout.count();
        j["hide_vault_dir"] = config.hide_vault_dir;
        j["secure_delete"] = config.secure_delete;
        j["secure_delete_passes"] = config.secure_delete_passes;

        std::string json_str = j.dump();
        std::vector<uint8_t> data(json_str.begin(), json_str.end());
        
        std::vector<uint8_t> encrypted = encryption_.encryptData(data, key_, iv_);
        if (encrypted.empty()) {
            last_error_ = "Encryption failed";
            return false;
        }

        std::string filepath = METADATA_DIR + "/" + CONFIG_DIR + "/" + vault_id + ".conf";
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            last_error_ = "Failed to open config file";
            return false;
        }

        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return true;
    }

    std::optional<VaultConfig> loadVaultConfig(const std::string& vault_id) {
        std::string filepath = METADATA_DIR + "/" + CONFIG_DIR + "/" + vault_id + ".conf";
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            last_error_ = "Failed to open config file";
            return std::nullopt;
        }

        auto size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> encrypted(size);
        file.read(reinterpret_cast<char*>(encrypted.data()), size);

        std::vector<uint8_t> decrypted = encryption_.decryptData(encrypted, key_, iv_);
        if (decrypted.empty()) {
            last_error_ = "Decryption failed";
            return std::nullopt;
        }

        std::string json_str(decrypted.begin(), decrypted.end());
        json j = json::parse(json_str);

        VaultConfig config;
        config.auto_lock = j["auto_lock"];
        config.lock_timeout = seconds(j["lock_timeout"].get<int64_t>());
        config.clear_clipboard = j["clear_clipboard"];
        config.clipboard_timeout = seconds(j["clipboard_timeout"].get<int64_t>());
        config.hide_vault_dir = j["hide_vault_dir"];
        config.secure_delete = j["secure_delete"];
        config.secure_delete_passes = j["secure_delete_passes"];

        return config;
    }

    bool setupPasswordRecovery(const std::string& vault_id, const RecoveryInfo& recovery_info) {
        json j;
        j["vault_id"] = recovery_info.vault_id;
        j["created_time"] = system_clock::to_time_t(recovery_info.created_time);
        j["last_used"] = system_clock::to_time_t(recovery_info.last_used);
        j["attempts_remaining"] = recovery_info.attempts_remaining;
        // Convert vectors to base64 strings for JSON serialization
        j["recovery_key"] = base64_encode(recovery_info.recovery_key);
        j["recovery_iv"] = base64_encode(recovery_info.recovery_iv);
        
        json questions_array = json::array();
        for (const auto& question : recovery_info.questions) {
            json q;
            q["question_id"] = question.question_id;
            q["question_text"] = question.question_text;
            // Convert vectors to base64 strings for JSON serialization
            q["answer_hash"] = base64_encode(question.answer_hash);
            q["salt"] = base64_encode(question.salt);
            questions_array.push_back(q);
        }
        j["questions"] = questions_array;

        std::string json_str = j.dump();
        std::vector<uint8_t> data(json_str.begin(), json_str.end());
        
        std::vector<uint8_t> encrypted = encryption_.encryptData(data, key_, iv_);
        if (encrypted.empty()) {
            last_error_ = "Encryption failed";
            return false;
        }

        std::string filepath = METADATA_DIR + "/" + RECOVERY_DIR + "/" + vault_id + ".recovery";
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            last_error_ = "Failed to open recovery file";
            return false;
        }

        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return true;
    }

    std::vector<uint8_t> verifyRecoveryAnswers(const std::string& vault_id, const std::vector<std::string>& answers) {
        std::string filepath = METADATA_DIR + "/" + RECOVERY_DIR + "/" + vault_id + ".recovery";
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            last_error_ = "Recovery file not found";
            return std::vector<uint8_t>();
        }

        auto size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> encrypted(size);
        file.read(reinterpret_cast<char*>(encrypted.data()), size);

        std::vector<uint8_t> decrypted = encryption_.decryptData(encrypted, key_, iv_);
        if (decrypted.empty()) {
            last_error_ = "Decryption failed";
            return std::vector<uint8_t>();
        }

        std::string json_str(decrypted.begin(), decrypted.end());
        json j = json::parse(json_str);

        // Check if attempts remaining
        uint32_t attempts_remaining = j["attempts_remaining"];
        if (attempts_remaining == 0) {
            last_error_ = "No recovery attempts remaining";
            return std::vector<uint8_t>();
        }

        // Verify answers
        json questions = j["questions"];
        if (questions.size() != answers.size()) {
            last_error_ = "Number of answers does not match number of questions";
            return std::vector<uint8_t>();
        }

        for (size_t i = 0; i < questions.size(); ++i) {
            std::string salt_str = questions[i]["salt"];
            std::vector<uint8_t> salt = base64_decode(salt_str);
            
            std::string answer_hash_str = questions[i]["answer_hash"];
            std::vector<uint8_t> expected_hash = base64_decode(answer_hash_str);
            
            // Hash the provided answer
            std::vector<uint8_t> answer_bytes(answers[i].begin(), answers[i].end());
            std::vector<uint8_t> derived_key = encryption_.deriveKeyFromPassword(answers[i], salt);
            
            // Simple hash comparison (in real implementation, use proper hash function)
            if (derived_key != expected_hash) {
                // Decrement attempts and update file
                attempts_remaining--;
                j["attempts_remaining"] = attempts_remaining;
                j["last_used"] = system_clock::to_time_t(system_clock::now());
                
                std::string updated_json = j.dump();
                std::vector<uint8_t> updated_data(updated_json.begin(), updated_json.end());
                std::vector<uint8_t> updated_encrypted = encryption_.encryptData(updated_data, key_, iv_);
                
                std::ofstream update_file(filepath, std::ios::binary);
                update_file.write(reinterpret_cast<const char*>(updated_encrypted.data()), updated_encrypted.size());
                
                last_error_ = "Incorrect answer. Attempts remaining: " + std::to_string(attempts_remaining);
                return std::vector<uint8_t>();
            }
        }

        // All answers correct, return recovery key
        std::string recovery_key_str = j["recovery_key"];
        return base64_decode(recovery_key_str);
    }

    bool hasPasswordRecovery(const std::string& vault_id) {
        std::string filepath = METADATA_DIR + "/" + RECOVERY_DIR + "/" + vault_id + ".recovery";
        return fs_.exists(filepath);
    }

    std::vector<RecoveryQuestion> getRecoveryQuestions(const std::string& vault_id) {
        std::string filepath = METADATA_DIR + "/" + RECOVERY_DIR + "/" + vault_id + ".recovery";
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            last_error_ = "Recovery file not found";
            return std::vector<RecoveryQuestion>();
        }

        auto size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> encrypted(size);
        file.read(reinterpret_cast<char*>(encrypted.data()), size);

        std::vector<uint8_t> decrypted = encryption_.decryptData(encrypted, key_, iv_);
        if (decrypted.empty()) {
            last_error_ = "Decryption failed";
            return std::vector<RecoveryQuestion>();
        }

        std::string json_str(decrypted.begin(), decrypted.end());
        json j = json::parse(json_str);

        std::vector<RecoveryQuestion> questions;
        json questions_array = j["questions"];
        for (const auto& q : questions_array) {
            RecoveryQuestion question;
            question.question_id = q["question_id"];
            question.question_text = q["question_text"];
            
            std::string salt_str = q["salt"];
            question.salt = base64_decode(salt_str);
            
            std::string hash_str = q["answer_hash"];
            question.answer_hash = base64_decode(hash_str);
            
            questions.push_back(question);
        }

        return questions;
    }

    bool removePasswordRecovery(const std::string& vault_id) {
        std::string filepath = METADATA_DIR + "/" + RECOVERY_DIR + "/" + vault_id + ".recovery";
        return fs_.remove(filepath, false);
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    EncryptionEngine encryption_;
    fs::FileSystem fs_;
    std::vector<uint8_t> key_;
    std::vector<uint8_t> iv_;
    std::string last_error_;
};

// Public interface implementation
SecureStorage::SecureStorage() : pimpl(std::make_unique<Implementation>()) {}
SecureStorage::~SecureStorage() = default;

bool SecureStorage::initialize(const std::vector<uint8_t>& master_key) {
    return pimpl->initialize(master_key);
}

bool SecureStorage::saveVaultMetadata(const VaultMetadata& metadata) {
    return pimpl->saveVaultMetadata(metadata);
}

std::optional<VaultMetadata> SecureStorage::loadVaultMetadata(const std::string& vault_id) {
    return pimpl->loadVaultMetadata(vault_id);
}

std::vector<std::string> SecureStorage::listVaults() {
    return pimpl->listVaults();
}

bool SecureStorage::deleteVaultMetadata(const std::string& vault_id) {
    return pimpl->deleteVaultMetadata(vault_id);
}

bool SecureStorage::saveVaultConfig(const std::string& vault_id, const VaultConfig& config) {
    return pimpl->saveVaultConfig(vault_id, config);
}

std::optional<VaultConfig> SecureStorage::loadVaultConfig(const std::string& vault_id) {
    return pimpl->loadVaultConfig(vault_id);
}

bool SecureStorage::setupPasswordRecovery(const std::string& vault_id, const RecoveryInfo& recovery_info) {
    return pimpl->setupPasswordRecovery(vault_id, recovery_info);
}

std::vector<uint8_t> SecureStorage::verifyRecoveryAnswers(const std::string& vault_id, const std::vector<std::string>& answers) {
    return pimpl->verifyRecoveryAnswers(vault_id, answers);
}

bool SecureStorage::hasPasswordRecovery(const std::string& vault_id) {
    return pimpl->hasPasswordRecovery(vault_id);
}

std::vector<RecoveryQuestion> SecureStorage::getRecoveryQuestions(const std::string& vault_id) {
    return pimpl->getRecoveryQuestions(vault_id);
}

bool SecureStorage::removePasswordRecovery(const std::string& vault_id) {
    return pimpl->removePasswordRecovery(vault_id);
}

std::string SecureStorage::getLastError() const {
    return pimpl->getLastError();
}

} // namespace storage
} // namespace phantom_vault 