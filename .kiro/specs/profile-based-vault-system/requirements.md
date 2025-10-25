# Requirements Document

## Introduction

This document specifies the requirements for redesigning PhantomVault from a single-user vault system to a multi-profile system where each profile operates independently with separate master keys, recovery keys, and folder management. The system will enforce security through admin/sudo privileges for profile creation and require master key verification for all vault operations.

## Glossary

- **PhantomVault_System**: The complete application including GUI, service, and all components
- **Profile**: A separate user context with its own master key, recovery key, and associated folders
- **Master_Key**: The primary authentication credential for a specific profile
- **Recovery_Key**: A backup authentication method provided only during profile creation
- **Vault_Folder**: A folder that has been added to a profile and is managed by the system
- **Admin_Mode**: Running the application with elevated privileges (sudo/admin rights)
- **Profile_Context**: The currently active profile that determines which folders are accessible
- **Invisible_Unlock**: The Ctrl+Alt+V hotkey sequence that activates keyboard pattern detection for authentication
- **Keyboard_Pattern_Detection**: System capability to detect authentication patterns from any subsequent keyboard input
- **Platform_Support**: The level of invisible keyboard logging support available on the current platform (X11, Wayland, macOS, Windows)

## Requirements

### Requirement 1

**User Story:** As a system administrator, I want to ensure only authorized users can create profiles, so that the system maintains security and prevents unauthorized access to other users' folders.

#### Acceptance Criteria

1. WHEN THE PhantomVault_System starts without Admin_Mode, THE PhantomVault_System SHALL display existing profiles without profile creation options
2. WHEN THE PhantomVault_System starts with Admin_Mode, THE PhantomVault_System SHALL enable profile creation functionality
3. IF a user attempts profile creation without Admin_Mode, THEN THE PhantomVault_System SHALL display an error message requiring elevated privileges
4. THE PhantomVault_System SHALL verify Admin_Mode privileges before allowing any profile creation operations

### Requirement 2

**User Story:** As a user, I want to create separate profiles for different purposes (school, home, personal users), so that I can organize and secure my folders independently.

#### Acceptance Criteria

1. WHEN creating a new profile in Admin_Mode, THE PhantomVault_System SHALL require a unique profile name
2. WHEN creating a new profile, THE PhantomVault_System SHALL generate a unique Master_Key for that profile
3. WHEN creating a new profile, THE PhantomVault_System SHALL provide a Recovery_Key that is displayed only once
4. THE PhantomVault_System SHALL store each profile with separate encryption keys and folder associations
5. THE PhantomVault_System SHALL prevent duplicate profile names within the same system

### Requirement 3

**User Story:** As a user, I want to select and authenticate into a specific profile, so that I can access only the folders associated with that profile.

#### Acceptance Criteria

1. WHEN THE PhantomVault_System starts, THE PhantomVault_System SHALL display all available profiles for selection
2. WHEN a user selects a profile, THE PhantomVault_System SHALL require Master_Key verification before activation
3. WHEN Master_Key verification succeeds, THE PhantomVault_System SHALL set the Profile_Context to the selected profile
4. WHILE a Profile_Context is active, THE PhantomVault_System SHALL display only folders associated with that profile
5. THE PhantomVault_System SHALL maintain Profile_Context until user switches profiles or application closes

### Requirement 4

**User Story:** As a user, I want to add folders to my active profile with master key verification, so that only I can modify my profile's folder list.

#### Acceptance Criteria

1. WHEN a user attempts to add a folder, THE PhantomVault_System SHALL require Master_Key verification for the active Profile_Context
2. WHEN Master_Key verification succeeds, THE PhantomVault_System SHALL allow folder selection and addition
3. WHEN a folder is successfully added, THE PhantomVault_System SHALL immediately lock and hide the folder from its original location
4. THE PhantomVault_System SHALL associate the added folder exclusively with the active Profile_Context
5. THE PhantomVault_System SHALL prevent adding the same folder to multiple profiles

### Requirement 5

**User Story:** As a user, I want folders to be automatically locked and hidden when added to a profile, so that my sensitive data is immediately protected.

#### Acceptance Criteria

1. WHEN a folder is added to a profile, THE PhantomVault_System SHALL immediately encrypt and hide the folder
2. THE PhantomVault_System SHALL move the folder to a secure location managed by the system
3. THE PhantomVault_System SHALL remove the folder from its original filesystem location
4. THE PhantomVault_System SHALL maintain metadata linking the folder to its Profile_Context
5. THE PhantomVault_System SHALL ensure the folder remains inaccessible without proper authentication

### Requirement 6

**User Story:** As a user, I want to unlock folders using the invisible hotkey system with profile-specific passwords, so that I can access my data without revealing the application interface.

#### Acceptance Criteria

1. WHEN the Invisible_Unlock sequence (Ctrl+Alt+V) is triggered, THE PhantomVault_System SHALL activate Keyboard_Pattern_Detection mode
2. WHILE Keyboard_Pattern_Detection is active, THE PhantomVault_System SHALL monitor all keyboard input for authentication patterns
3. WHEN a valid Master_Key pattern is detected in any text input, THE PhantomVault_System SHALL unlock all folders associated with that profile
4. THE PhantomVault_System SHALL support T+password format for temporary unlock (e.g., "t1234" in "hey this is a t1234 testing")
5. THE PhantomVault_System SHALL support P+password format for permanent unlock (e.g., "p1234" in any text context)
6. WHERE Platform_Support allows invisible logging, THE PhantomVault_System SHALL extract patterns from any keyboard input
7. WHERE Platform_Support requires explicit input, THE PhantomVault_System SHALL prompt for password entry after Ctrl+Alt+V

### Requirement 7

**User Story:** As a user, I want complete separation between profiles, so that accessing one profile never exposes data from another profile.

#### Acceptance Criteria

1. THE PhantomVault_System SHALL maintain separate encryption keys for each profile
2. THE PhantomVault_System SHALL prevent cross-profile folder access under any circumstances
3. WHEN switching profiles, THE PhantomVault_System SHALL lock all folders from the previous profile
4. THE PhantomVault_System SHALL ensure profile metadata and folder associations remain isolated
5. THE PhantomVault_System SHALL verify Profile_Context before any folder operation

### Requirement 8

**User Story:** As a user, I want to access recovery functionality through the GUI settings, so that I can recover my master key when needed without compromising security.

#### Acceptance Criteria

1. WHEN creating a profile, THE PhantomVault_System SHALL generate and display a Recovery_Key exactly once
2. THE PhantomVault_System SHALL require the user to confirm they have saved the Recovery_Key before proceeding
3. THE PhantomVault_System SHALL provide a recovery section in the settings interface
4. WHEN a Recovery_Key is entered in settings, THE PhantomVault_System SHALL display the associated Master_Key
5. THE PhantomVault_System SHALL close the recovery interface after Master_Key confirmation
6. THE PhantomVault_System SHALL NOT support Ctrl+Alt+R recovery hotkey functionality

### Requirement 9

**User Story:** As a user, I want platform-specific guidance for the invisible unlock feature, so that I understand how the system works on my operating system.

#### Acceptance Criteria

1. THE PhantomVault_System SHALL detect the current platform (X11, Wayland, macOS, Windows)
2. WHERE Platform_Support includes invisible keyboard logging, THE PhantomVault_System SHALL inform users that patterns are detected from any keyboard input
3. WHERE Platform_Support requires explicit input, THE PhantomVault_System SHALL inform users to enter password after Ctrl+Alt+V and press Enter
4. THE PhantomVault_System SHALL provide platform-specific instructions in help/support/guide sections
5. THE PhantomVault_System SHALL adapt authentication behavior based on Platform_Support capabilities