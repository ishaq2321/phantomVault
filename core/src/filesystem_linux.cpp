#include "phantom_vault/filesystem.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <stdexcept>

namespace phantom_vault {
namespace fs {

class FileSystem::Implementation {
public:
    Implementation() : last_error_() {}

    std::error_code getLastError() const { return last_error_; }

    bool hide(const std::filesystem::path& path) {
        // On Linux, we prepend a dot to make files hidden
        if (!exists(path)) {
            last_error_ = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        std::filesystem::path parent = path.parent_path();
        std::string filename = path.filename().string();
        
        if (filename[0] != '.') {
            std::filesystem::path new_path = parent / ("." + filename);
            return move(path, new_path);
        }
        return true;
    }

    bool unhide(const std::filesystem::path& path) {
        if (!exists(path)) {
            last_error_ = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        std::filesystem::path parent = path.parent_path();
        std::string filename = path.filename().string();
        
        if (filename[0] == '.') {
            std::filesystem::path new_path = parent / filename.substr(1);
            return move(path, new_path);
        }
        return true;
    }

    bool setAttributes(const std::filesystem::path& path, const FileAttributes& attrs) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            last_error_ = std::error_code(errno, std::system_category());
            return false;
        }

        mode_t mode = st.st_mode;
        if (attrs.readonly) {
            mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
        } else {
            mode |= (S_IWUSR);
        }

        if (chmod(path.c_str(), mode) != 0) {
            last_error_ = std::error_code(errno, std::system_category());
            return false;
        }

        return setTimestamps(path, attrs.created_time, attrs.modified_time, attrs.accessed_time);
    }

    bool getAttributes(const std::filesystem::path& path, FileAttributes& attrs) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            last_error_ = std::error_code(errno, std::system_category());
            return false;
        }

        attrs.readonly = !(st.st_mode & S_IWUSR);
        attrs.hidden = path.filename().string()[0] == '.';
        attrs.system = false; // Linux doesn't have system attribute
        attrs.permissions = static_cast<std::filesystem::perms>(st.st_mode & 07777);
        
        attrs.created_time = std::chrono::system_clock::from_time_t(st.st_ctime);
        attrs.modified_time = std::chrono::system_clock::from_time_t(st.st_mtime);
        attrs.accessed_time = std::chrono::system_clock::from_time_t(st.st_atime);

        return true;
    }

    bool setTimestamps(
        const std::filesystem::path& path,
        const std::chrono::system_clock::time_point& created,
        const std::chrono::system_clock::time_point& modified,
        const std::chrono::system_clock::time_point& accessed) {
        
        struct utimbuf times;
        times.actime = std::chrono::system_clock::to_time_t(accessed);
        times.modtime = std::chrono::system_clock::to_time_t(modified);
        
        if (utime(path.c_str(), &times) != 0) {
            last_error_ = std::error_code(errno, std::system_category());
            return false;
        }
        
        return true;
    }

    bool exists(const std::filesystem::path& path) const {
        return std::filesystem::exists(path);
    }

    bool isHidden(const std::filesystem::path& path) const {
        return path.filename().string()[0] == '.';
    }

    bool createDirectories(const std::filesystem::path& path) {
        std::error_code ec;
        bool result = std::filesystem::create_directories(path, ec);
        if (ec) {
            last_error_ = ec;
            return false;
        }
        return result;
    }

    bool remove(const std::filesystem::path& path, bool recursive) {
        std::error_code ec;
        bool result;
        
        if (recursive) {
            result = std::filesystem::remove_all(path, ec) > 0;
        } else {
            result = std::filesystem::remove(path, ec);
        }
        
        if (ec) {
            last_error_ = ec;
            return false;
        }
        return result;
    }

    bool move(const std::filesystem::path& from, const std::filesystem::path& to) {
        std::error_code ec;
        std::filesystem::rename(from, to, ec);
        if (ec) {
            last_error_ = ec;
            return false;
        }
        return true;
    }

    bool copy(const std::filesystem::path& from, const std::filesystem::path& to, bool recursive) {
        std::error_code ec;
        if (recursive) {
            std::filesystem::copy(from, to, 
                std::filesystem::copy_options::recursive |
                std::filesystem::copy_options::copy_symlinks, ec);
        } else {
            std::filesystem::copy(from, to, 
                std::filesystem::copy_options::copy_symlinks, ec);
        }
        
        if (ec) {
            last_error_ = ec;
            return false;
        }
        return true;
    }

private:
    std::error_code last_error_;
};

// Public interface implementation
FileSystem::FileSystem() : pimpl(std::make_unique<Implementation>()) {}
FileSystem::~FileSystem() = default;

bool FileSystem::hide(const std::filesystem::path& path) { return pimpl->hide(path); }
bool FileSystem::unhide(const std::filesystem::path& path) { return pimpl->unhide(path); }
bool FileSystem::setAttributes(const std::filesystem::path& path, const FileAttributes& attrs) { return pimpl->setAttributes(path, attrs); }
bool FileSystem::getAttributes(const std::filesystem::path& path, FileAttributes& attrs) { return pimpl->getAttributes(path, attrs); }
bool FileSystem::setTimestamps(const std::filesystem::path& path, const std::chrono::system_clock::time_point& created,
    const std::chrono::system_clock::time_point& modified, const std::chrono::system_clock::time_point& accessed) {
    return pimpl->setTimestamps(path, created, modified, accessed);
}
bool FileSystem::exists(const std::filesystem::path& path) const { return pimpl->exists(path); }
bool FileSystem::isHidden(const std::filesystem::path& path) const { return pimpl->isHidden(path); }
bool FileSystem::createDirectories(const std::filesystem::path& path) { return pimpl->createDirectories(path); }
bool FileSystem::remove(const std::filesystem::path& path, bool recursive) { return pimpl->remove(path, recursive); }
bool FileSystem::move(const std::filesystem::path& from, const std::filesystem::path& to) { return pimpl->move(from, to); }
bool FileSystem::copy(const std::filesystem::path& from, const std::filesystem::path& to, bool recursive) { return pimpl->copy(from, to, recursive); }
std::error_code FileSystem::getLastError() const { return pimpl->getLastError(); }

} // namespace fs
} // namespace phantom_vault 