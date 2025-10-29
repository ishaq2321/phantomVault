/**
 * PhantomVault - Unified Application Entry Point
 * 
 * Single entry point that replaces all duplicate main() functions.
 * Preserves existing ServiceManager architecture while providing unified interface.
 */

#include "phantomvault_application.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        PhantomVaultApplication app;
        return app.run(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}