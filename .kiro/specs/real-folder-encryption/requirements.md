# Real Folder Encryption - Requirements Document

## Introduction

This specification defines the requirements for implementing actual AES-256 folder encryption in PhantomVault's multi-profile system. Currently, the system only creates marker files and changes permissions without any real encryption. This feature will implement genuine cryptographic protection for user folders and files across multiple profiles, with platform-specific unlock mechanisms, sophisticated access control, and system-wide keyboard detection running as a privileged native service.

## Glossary

- **Encryption_Engine**: The core component responsible for AES-256 encryption/decryption operations
- **Profile_Vault**: Encrypted storage location where a specific profile's protected folders are stored
- **Master_Key**: Profile-specific authentication credential for accessing that profile's encrypted folders
- **Profile_Manager**: Component managing multiple user profiles with separate master keys and isolated vaults
- **Native_Service**: Background service that runs with elevated privileges and handles keyboard detection system-wide
- **Keyboard_Detector**: Component that monitors for master key input sequences globally, independent of GUI
- **Platform_Adapter**: Component that provides platform-specific unlock mechanisms and capability detection
- **Temporary_Unlock**: Unlock mode where folders remain accessible until system lock, reboot, or manual re-lock
- **Permanent_Unlock**: Unlock mode where folders are permanently restored and removed from vault tracking
- **Recovery_Key**: Cryptographically secure key that allows recovery of a profile's master key
- **Admin_Privileges**: Elevated system privileges required for application access and vault operations
- **Folder_Hiding**: Complete concealment of folders requiring elevated privileges to access or delete
- **Encryption_Metadata**: Information about encryption parameters (IV, salt, algorithm) stored with encrypted data
- **File_Processor**: Component that handles individual file encryption within folders
- **Key_Derivation_Function**: PBKDF2 implementation for deriving encryption keys from profile master passwords

## Requirements

### Requirement 1: Multi-Profile Folder Encryption System

**User Story:** As a user with multiple security contexts, I want separate profiles with individual master keys so that I can isolate different types of sensitive data with independent encryption.

#### Acceptance Criteria

1. WHEN a user creates a profile, THE Profile_Manager SHALL generate a unique profile identifier and associate it with a master key
2. WHEN a user locks a folder to a profile, THE Encryption_Engine SHALL encrypt all files using AES-256-CBC with that profile's derived encryption key
3. WHEN multiple profiles exist, THE system SHALL maintain completely separate Profile_Vaults for each profile's encrypted data
4. WHEN a master key is provided, THE system SHALL only decrypt folders belonging to that specific profile
5. WHERE profiles have different master keys, THE system SHALL prevent cross-profile access even with valid authentication

### Requirement 2: Privileged Native Service with Keyboard Detection

**User Story:** As a user, I want system-wide keyboard detection that works independently of the GUI so that I can access my folders from anywhere without opening applications.

#### Acceptance Criteria

1. THE Native_Service SHALL run with elevated privileges to enable system-wide keyboard monitoring and folder protection
2. WHEN the Native_Service starts, THE Keyboard_Detector SHALL monitor for master key input sequences globally across all applications
3. WHEN a master key sequence is detected, THE Native_Service SHALL unlock corresponding profile folders without requiring GUI interaction
4. WHERE GUI is not accessible, THE Native_Service SHALL still provide full keyboard-based unlock functionality
5. WHILE monitoring keyboards, THE Native_Service SHALL only capture and process master key sequences, ignoring all other input

### Requirement 3: Complete Folder Hiding and Protection

**User Story:** As a security-conscious user, I want my protected folders to be completely hidden and inaccessible without proper authentication so that they cannot be discovered or tampered with.

#### Acceptance Criteria

1. WHEN folders are locked, THE system SHALL completely hide them from file system access without elevated privileges
2. WHEN attempting to access hidden folders without authentication, THE system SHALL deny access even to administrators without proper credentials
3. WHEN folders are in Profile_Vault, THE system SHALL prevent deletion, modification, or discovery of vault contents without master key
4. WHERE elevated privileges are required, THE system SHALL enforce authentication before granting any vault access
5. WHILE folders are hidden, THE system SHALL maintain no visible traces of their existence in the original locations

### Requirement 4: Platform-Specific Unlock Mechanisms

**User Story:** As a user on different operating systems, I want platform-appropriate fallback methods to unlock my folders when keyboard detection is not available.

#### Acceptance Criteria

1. WHEN keyboard detection is unavailable, THE Platform_Adapter SHALL provide platform-specific unlock alternatives
2. WHEN on systems supporting notifications, THE system SHALL offer notification-based password input for unlocking
3. WHEN right-clicking on vault locations, THE system SHALL provide context menu password input where supported by platform
4. WHERE platform limitations exist, THE Platform_Adapter SHALL detect capabilities and offer appropriate unlock methods
5. WHILE providing fallbacks, THE system SHALL maintain the same security level as keyboard-based unlocking

### Requirement 5: Temporary vs Permanent Unlock Modes

**User Story:** As a user, I want different unlock modes so that I can choose between temporary access that auto-locks and permanent restoration of my folders.

#### Acceptance Criteria

1. WHEN performing temporary unlock, THE system SHALL make folders accessible until system lock, reboot, or manual re-lock
2. WHEN the same master key is detected again during temporary unlock, THE system SHALL re-lock the temporarily unlocked folders
3. WHEN system locks or reboots, THE system SHALL automatically re-lock all temporarily unlocked folders
4. WHEN performing permanent unlock, THE system SHALL restore folders to original locations and remove them from vault tracking
5. WHERE permanent unlock is chosen, THE system SHALL no longer track or protect those folders

### Requirement 6: AES-256 File-Level Encryption Implementation

**User Story:** As a user, I want each file in my protected folders to be individually encrypted with strong cryptography so that my data is genuinely secure.

#### Acceptance Criteria

1. WHEN encrypting files, THE File_Processor SHALL encrypt each file individually using AES-256-CBC with unique IVs
2. WHEN deriving encryption keys, THE Key_Derivation_Function SHALL use PBKDF2 with SHA-256 and minimum 100,000 iterations
3. WHEN processing large files, THE File_Processor SHALL encrypt in chunks to manage memory efficiently
4. WHEN storing encrypted files, THE system SHALL include cryptographic metadata (IV, salt, algorithm parameters) with each file
5. WHILE maintaining security, THE system SHALL preserve original file metadata (timestamps, permissions) in encrypted form

### Requirement 7: Profile-Specific Access Control

**User Story:** As a user with multiple profiles, I want strict isolation between profiles so that each master key only accesses its designated folders.

#### Acceptance Criteria

1. WHEN a master key is provided, THE system SHALL identify the corresponding profile and only unlock that profile's folders
2. WHEN multiple profiles have folders, THE system SHALL maintain complete isolation between profile vaults
3. WHEN authenticating with one profile's master key, THE system SHALL not provide any access to other profiles' data
4. WHERE profile authentication fails, THE system SHALL provide no information about other existing profiles
5. WHILE managing multiple profiles, THE system SHALL prevent any cross-contamination of encryption keys or data

### Requirement 8: Platform Detection and Capability Adaptation

**User Story:** As a user on different platforms, I want the system to detect my platform's capabilities and provide appropriate security features.

#### Acceptance Criteria

1. WHEN starting up, THE Platform_Adapter SHALL detect the current operating system and available security features
2. WHEN platform supports advanced keyboard monitoring, THE system SHALL enable full invisible keyboard detection
3. WHEN platform has limitations, THE Platform_Adapter SHALL gracefully degrade to available unlock methods
4. WHERE certain features are unavailable, THE system SHALL inform users of platform-specific limitations
5. WHILE adapting to platforms, THE system SHALL maintain maximum security possible within platform constraints

### Requirement 9: Error Handling and Recovery

**User Story:** As a user, I want robust error handling during encryption operations so that my data is never lost or corrupted, even across different profiles.

#### Acceptance Criteria

1. WHEN encryption fails for any profile, THE system SHALL maintain original files until encryption is verified complete
2. WHEN profile authentication fails, THE system SHALL provide clear error messages without exposing profile information
3. IF vault corruption is detected, THEN THE system SHALL attempt recovery using profile-specific backup mechanisms
4. WHEN system crashes during operations, THE system SHALL provide recovery tools to restore consistent state
5. WHILE handling errors, THE system SHALL maintain profile isolation and never expose cross-profile information

### Requirement 10: Recovery Key System and Password Management

**User Story:** As a user, I want to recover my master key using a recovery key and change my profile passwords so that I can regain access if I forget my password and maintain security through key rotation.

#### Acceptance Criteria

1. WHEN a profile is created, THE system SHALL generate a cryptographically secure Recovery_Key associated with that profile's master key
2. WHEN a user provides a Recovery_Key in settings, THE system SHALL return only the master key associated with that specific recovery key
3. WHEN a user changes a profile password, THE system SHALL generate a new Recovery_Key and invalidate the previous one
4. WHEN password change is complete, THE system SHALL provide the new Recovery_Key to the user and update all encrypted data with new key derivation
5. WHERE recovery key is used, THE system SHALL maintain the same security level as direct master key authentication

### Requirement 11: Admin Privilege Enforcement

**User Story:** As a system administrator, I want the application to require elevated privileges so that vault operations and folder hiding are properly secured and cannot be bypassed.

#### Acceptance Criteria

1. THE application SHALL require admin/sudo privileges to launch from desktop or terminal
2. WHEN launched without admin privileges, THE system SHALL refuse to start and display appropriate error message
3. WHEN running with admin privileges, THE system SHALL be able to hide folders completely from non-privileged access
4. WHEN vault operations are performed, THE system SHALL use elevated privileges to ensure folders cannot be deleted or accessed without authentication
5. WHERE admin privileges are lost during operation, THE system SHALL gracefully handle privilege changes and maintain security

### Requirement 12: Cryptographic Standards and Security

**User Story:** As a security professional, I want the multi-profile encryption system to follow established cryptographic standards with proper key isolation.

#### Acceptance Criteria

1. THE Encryption_Engine SHALL use only FIPS-approved cryptographic algorithms (AES-256, SHA-256, PBKDF2)
2. THE system SHALL generate cryptographically secure random numbers for all cryptographic operations per profile
3. THE system SHALL implement proper authenticated encryption or integrity protection for all encrypted data
4. THE system SHALL ensure complete cryptographic isolation between different profiles' key material
5. THE system SHALL follow current NIST guidelines for key derivation with profile-specific salts and parameters