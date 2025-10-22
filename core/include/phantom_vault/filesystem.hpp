#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <memory>
#include <system_error>

namespace phantom_vault {
namespace fs {

/**
 * @brief File attributes structure
 */
struct PHANTOM_VAULT_EXPORT FileAttributes {
    bool hidden;
    bool readonly;
    bool system;
    std::filesystem::perms permissions;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point modified_time;
    std::chrono::system_clock::time_point accessed_time;
};

/**
 * @brief Platform-independent file system operations
 */
class PHANTOM_VAULT_EXPORT FileSystem {
public:
    /**
     * @brief Constructor
     */
    FileSystem();

    /**
     * @brief Destructor
     */
    ~FileSystem();

    /**
     * @brief Hide a file or directory
     * @param path Path to hide
     * @return true if successful, false otherwise
     */
    bool hide(const std::filesystem::path& path);

    /**
     * @brief Unhide a file or directory
     * @param path Path to unhide
     * @return true if successful, false otherwise
     */
    bool unhide(const std::filesystem::path& path);

    /**
     * @brief Set file attributes
     * @param path Target path
     * @param attrs Attributes to set
     * @return true if successful, false otherwise
     */
    bool setAttributes(const std::filesystem::path& path, const FileAttributes& attrs);

    /**
     * @brief Get file attributes
     * @param path Target path
     * @param[out] attrs Retrieved attributes
     * @return true if successful, false otherwise
     */
    bool getAttributes(const std::filesystem::path& path, FileAttributes& attrs);

    /**
     * @brief Set file timestamps
     * @param path Target path
     * @param created Creation time
     * @param modified Modification time
     * @param accessed Access time
     * @return true if successful, false otherwise
     */
    bool setTimestamps(
        const std::filesystem::path& path,
        const std::chrono::system_clock::time_point& created,
        const std::chrono::system_clock::time_point& modified,
        const std::chrono::system_clock::time_point& accessed
    );

    /**
     * @brief Check if a path exists
     * @param path Path to check
     * @return true if exists, false otherwise
     */
    bool exists(const std::filesystem::path& path) const;

    /**
     * @brief Check if a path is hidden
     * @param path Path to check
     * @return true if hidden, false otherwise
     */
    bool isHidden(const std::filesystem::path& path) const;

    /**
     * @brief Create a directory with all parent directories if needed
     * @param path Directory to create
     * @return true if successful, false otherwise
     */
    bool createDirectories(const std::filesystem::path& path);

    /**
     * @brief Remove a file or directory
     * @param path Path to remove
     * @param recursive If true, remove directories recursively
     * @return true if successful, false otherwise
     */
    bool remove(const std::filesystem::path& path, bool recursive = false);

    /**
     * @brief Move/rename a file or directory
     * @param from Source path
     * @param to Destination path
     * @return true if successful, false otherwise
     */
    bool move(const std::filesystem::path& from, const std::filesystem::path& to);

    /**
     * @brief Copy a file or directory
     * @param from Source path
     * @param to Destination path
     * @param recursive If true, copy directories recursively
     * @return true if successful, false otherwise
     */
    bool copy(const std::filesystem::path& from, const std::filesystem::path& to, bool recursive = false);

    /**
     * @brief Get the last error that occurred
     * @return Error code with message
     */
    std::error_code getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class FileSystem

} // namespace fs
} // namespace phantom_vault 