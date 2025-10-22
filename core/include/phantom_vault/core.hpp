#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>

namespace phantom_vault {

/**
 * @brief Core class providing the main functionality of Phantom Vault
 */
class PHANTOM_VAULT_EXPORT Core {
public:
    /**
     * @brief Constructor
     */
    Core();

    /**
     * @brief Destructor
     */
    ~Core();

    /**
     * @brief Initialize the core library
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Get the library version
     * @return Version string
     */
    std::string getVersion() const;

    /**
     * @brief Check if the library is properly initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class Core

} // namespace phantom_vault 