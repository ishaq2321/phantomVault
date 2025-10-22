#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>
#include <vector>

namespace phantom_vault {

/**
 * @brief Class for managing process concealment
 */
class PHANTOM_VAULT_EXPORT ProcessConcealer {
public:
    /**
     * @brief Constructor
     */
    ProcessConcealer();

    /**
     * @brief Destructor
     */
    ~ProcessConcealer();

    /**
     * @brief Initialize the process concealer
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Change the visible process name
     * @param name New name to display in process listings
     * @return true if successful, false otherwise
     */
    bool setProcessName(const std::string& name);

    /**
     * @brief Hide the process from process listings
     * @return true if successful, false otherwise
     */
    bool hideProcess();

    /**
     * @brief Show the process in process listings
     * @return true if successful, false otherwise
     */
    bool showProcess();

    /**
     * @brief Check if the process is currently hidden
     * @return true if hidden, false otherwise
     */
    bool isHidden() const;

    /**
     * @brief Get the current visible process name
     * @return The current process name
     */
    std::string getCurrentProcessName() const;

    /**
     * @brief Get the original process name
     * @return The original process name
     */
    std::string getOriginalProcessName() const;

    /**
     * @brief Get the last error message
     * @return The last error message
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class ProcessConcealer

} // namespace phantom_vault 