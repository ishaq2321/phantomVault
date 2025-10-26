/**
 * PhantomVault Folder Security Manager
 * 
 * Manages folder locking, encryption, and security operations.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace phantomvault {

class FolderSecurityManager {
public:
    FolderSecurityManager();
    ~FolderSecurityManager();

    bool initialize();
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault