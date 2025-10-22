#include "phantom_vault/core.hpp"
#include "phantom_vault/version.hpp"

namespace phantom_vault {

class Core::Implementation {
public:
    Implementation() : initialized(false) {}
    
    bool initialize() {
        initialized = true;
        return true;
    }

    std::string getVersion() const {
        return version::VERSION;
    }

    bool isInitialized() const {
        return initialized;
    }

private:
    bool initialized;
};

Core::Core() : pimpl(std::make_unique<Implementation>()) {}
Core::~Core() = default;

bool Core::initialize() {
    return pimpl->initialize();
}

std::string Core::getVersion() const {
    return pimpl->getVersion();
}

bool Core::isInitialized() const {
    return pimpl->isInitialized();
}

} // namespace phantom_vault 