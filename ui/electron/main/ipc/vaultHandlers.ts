/**
 * Vault IPC Handlers
 * 
 * IPC handlers for vault operations in the Electron main process
 */

import { ipcMain, IpcMainInvokeEvent } from 'electron';
import { VaultConfig, VaultInfo, VaultOperationResult, ApiResponse } from '../../../src/types';
import { ServiceCommunicator } from '../services/ServiceCommunicator';
import { Logger } from '../utils/Logger';
import { ErrorHandler } from '../utils/ErrorHandler';
import { IPCErrorWrapper } from '../utils/IPCErrorWrapper';

export class VaultHandlers {
  private serviceCommunicator: ServiceCommunicator;
  private logger: Logger;
  private errorHandler: ErrorHandler;
  private errorWrapper: IPCErrorWrapper;

  constructor(serviceCommunicator: ServiceCommunicator, logger: Logger) {
    this.serviceCommunicator = serviceCommunicator;
    this.logger = logger;
    this.errorHandler = new ErrorHandler(logger);
    this.errorWrapper = new IPCErrorWrapper(this.errorHandler, logger);
    this.registerHandlers();
  }

  /**
   * Register all vault-related IPC handlers
   */
  private registerHandlers(): void {
    // Vault CRUD operations
    ipcMain.handle('vault:list', this.errorWrapper.createSecureHandler(
      this.handleListVaults.bind(this),
      { context: { operation: 'list-vaults', component: 'vault-handlers' } }
    ));
    
    ipcMain.handle('vault:create', this.errorWrapper.createSecureHandler(
      this.handleCreateVault.bind(this),
      { 
        context: { operation: 'create-vault', component: 'vault-handlers' },
        timeout: 30000,
        validator: this.validateCreateVaultArgs.bind(this)
      }
    ));
    
    ipcMain.handle('vault:update', this.errorWrapper.createSecureHandler(
      this.handleUpdateVault.bind(this),
      { 
        context: { operation: 'update-vault', component: 'vault-handlers' },
        timeout: 30000,
        validator: this.validateUpdateVaultArgs.bind(this)
      }
    ));
    
    ipcMain.handle('vault:delete', this.errorWrapper.createSecureHandler(
      this.handleDeleteVault.bind(this),
      { 
        context: { operation: 'delete-vault', component: 'vault-handlers' },
        timeout: 15000,
        validator: this.validateVaultIdArg.bind(this)
      }
    ));
    
    // Vault operations
    ipcMain.handle('vault:mount', this.errorWrapper.createSecureHandler(
      this.handleMountVault.bind(this),
      { 
        context: { operation: 'mount-vault', component: 'vault-handlers' },
        timeout: 30000,
        maxRetries: 2,
        retryDelay: 2000,
        validator: this.validateMountVaultArgs.bind(this)
      }
    ));
    
    ipcMain.handle('vault:unmount', this.errorWrapper.createSecureHandler(
      this.handleUnmountVault.bind(this),
      { 
        context: { operation: 'unmount-vault', component: 'vault-handlers' },
        timeout: 15000,
        validator: this.validateUnmountVaultArgs.bind(this)
      }
    ));
    
    ipcMain.handle('vault:status', this.errorWrapper.createSecureHandler(
      this.handleGetVaultStatus.bind(this),
      { 
        context: { operation: 'get-vault-status', component: 'vault-handlers' },
        timeout: 10000,
        validator: this.validateVaultIdArg.bind(this)
      }
    ));
    
    // Bulk operations
    ipcMain.handle('vault:bulk-mount', this.errorWrapper.createSecureHandler(
      this.handleBulkMountVaults.bind(this),
      { 
        context: { operation: 'bulk-mount-vaults', component: 'vault-handlers' },
        timeout: 60000,
        validator: this.validateBulkMountArgs.bind(this)
      }
    ));
    
    ipcMain.handle('vault:bulk-unmount', this.errorWrapper.createSecureHandler(
      this.handleBulkUnmountVaults.bind(this),
      { 
        context: { operation: 'bulk-unmount-vaults', component: 'vault-handlers' },
        timeout: 60000,
        validator: this.validateBulkUnmountArgs.bind(this)
      }
    ));
    
    // Validation
    ipcMain.handle('vault:validate-path', this.errorWrapper.createSecureHandler(
      this.handleValidatePath.bind(this),
      { 
        context: { operation: 'validate-path', component: 'vault-handlers' },
        timeout: 5000,
        rateLimit: { maxCalls: 20, windowMs: 60000 },
        validator: this.validatePathArg.bind(this)
      }
    ));
    
    ipcMain.handle('vault:validate-sequence', this.errorWrapper.createSecureHandler(
      this.handleValidateSequence.bind(this),
      { 
        context: { operation: 'validate-sequence', component: 'vault-handlers' },
        timeout: 5000,
        rateLimit: { maxCalls: 20, windowMs: 60000 },
        validator: this.validateSequenceArgs.bind(this)
      }
    ));
  }

  /**
   * Handle list vaults request
   */
  private async handleListVaults(event: IpcMainInvokeEvent): Promise<VaultInfo[]> {
    const vaults = await this.serviceCommunicator.listVaults();
    this.logger.info(`Found ${vaults.length} vaults`);
    return vaults;
  }

  /**
   * Handle create vault request
   */
  private async handleCreateVault(
    event: IpcMainInvokeEvent, 
    config: VaultConfig
  ): Promise<VaultInfo> {
    // Additional validation beyond basic argument validation
    const validationResult = await this.validateVaultConfig(config);
    if (!validationResult.isValid) {
      throw new Error(`Invalid vault configuration: ${validationResult.errors.join(', ')}`);
    }
    
    const vault = await this.serviceCommunicator.createVault(config);
    this.logger.info(`Successfully created vault "${vault.name}" with ID ${vault.id}`);
    return vault;
  }

  /**
   * Handle update vault request
   */
  private async handleUpdateVault(
    event: IpcMainInvokeEvent,
    vaultId: string,
    config: VaultConfig
  ): Promise<ApiResponse<VaultInfo>> {
    try {
      this.logger.info(`IPC: Updating vault ${vaultId}`);
      
      // Validate configuration
      const validationResult = await this.validateVaultConfig(config);
      if (!validationResult.isValid) {
        throw new Error(`Invalid vault configuration: ${validationResult.errors.join(', ')}`);
      }
      
      const vault = await this.serviceCommunicator.updateVault(vaultId, config);
      
      this.logger.info(`IPC: Successfully updated vault ${vaultId}`);
      return {
        success: true,
        data: vault,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to update vault';
      this.logger.error(`IPC: Failed to update vault ${vaultId}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle delete vault request
   */
  private async handleDeleteVault(
    event: IpcMainInvokeEvent,
    vaultId: string
  ): Promise<ApiResponse> {
    try {
      this.logger.info(`IPC: Deleting vault ${vaultId}`);
      
      await this.serviceCommunicator.deleteVault(vaultId);
      
      this.logger.info(`IPC: Successfully deleted vault ${vaultId}`);
      return {
        success: true,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to delete vault';
      this.logger.error(`IPC: Failed to delete vault ${vaultId}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle mount vault request
   */
  private async handleMountVault(
    event: IpcMainInvokeEvent,
    vaultId: string,
    password: string
  ): Promise<ApiResponse<VaultInfo>> {
    try {
      this.logger.info(`IPC: Mounting vault ${vaultId}`);
      
      const vault = await this.serviceCommunicator.mountVault(vaultId, password);
      
      this.logger.info(`IPC: Successfully mounted vault ${vaultId}`);
      return {
        success: true,
        data: vault,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to mount vault';
      this.logger.error(`IPC: Failed to mount vault ${vaultId}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle unmount vault request
   */
  private async handleUnmountVault(
    event: IpcMainInvokeEvent,
    vaultId: string,
    force: boolean = false
  ): Promise<ApiResponse> {
    try {
      this.logger.info(`IPC: Unmounting vault ${vaultId} (force: ${force})`);
      
      await this.serviceCommunicator.unmountVault(vaultId, force);
      
      this.logger.info(`IPC: Successfully unmounted vault ${vaultId}`);
      return {
        success: true,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to unmount vault';
      this.logger.error(`IPC: Failed to unmount vault ${vaultId}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle get vault status request
   */
  private async handleGetVaultStatus(
    event: IpcMainInvokeEvent,
    vaultId: string
  ): Promise<ApiResponse<VaultInfo>> {
    try {
      this.logger.debug(`IPC: Getting status for vault ${vaultId}`);
      
      const vault = await this.serviceCommunicator.getVaultStatus(vaultId);
      
      return {
        success: true,
        data: vault,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to get vault status';
      this.logger.error(`IPC: Failed to get status for vault ${vaultId}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle bulk mount vaults request
   */
  private async handleBulkMountVaults(
    event: IpcMainInvokeEvent,
    vaultIds: string[],
    passwords: Record<string, string>
  ): Promise<ApiResponse<VaultOperationResult[]>> {
    try {
      this.logger.info(`IPC: Bulk mounting ${vaultIds.length} vaults`);
      
      const results: VaultOperationResult[] = [];
      
      for (const vaultId of vaultIds) {
        try {
          const password = passwords[vaultId] || '';
          const vault = await this.serviceCommunicator.mountVault(vaultId, password);
          
          results.push({
            success: true,
            vault,
          });
          
          this.logger.info(`IPC: Successfully mounted vault ${vaultId} in bulk operation`);
        } catch (error) {
          const errorMessage = error instanceof Error ? error.message : 'Failed to mount vault';
          results.push({
            success: false,
            error: errorMessage,
          });
          
          this.logger.error(`IPC: Failed to mount vault ${vaultId} in bulk operation`, error);
        }
      }
      
      const successCount = results.filter(r => r.success).length;
      this.logger.info(`IPC: Bulk mount completed: ${successCount}/${vaultIds.length} successful`);
      
      return {
        success: true,
        data: results,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to perform bulk mount';
      this.logger.error('IPC: Failed to perform bulk mount', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle bulk unmount vaults request
   */
  private async handleBulkUnmountVaults(
    event: IpcMainInvokeEvent,
    vaultIds: string[],
    force: boolean = false
  ): Promise<ApiResponse<VaultOperationResult[]>> {
    try {
      this.logger.info(`IPC: Bulk unmounting ${vaultIds.length} vaults (force: ${force})`);
      
      const results: VaultOperationResult[] = [];
      
      for (const vaultId of vaultIds) {
        try {
          await this.serviceCommunicator.unmountVault(vaultId, force);
          
          results.push({
            success: true,
          });
          
          this.logger.info(`IPC: Successfully unmounted vault ${vaultId} in bulk operation`);
        } catch (error) {
          const errorMessage = error instanceof Error ? error.message : 'Failed to unmount vault';
          results.push({
            success: false,
            error: errorMessage,
          });
          
          this.logger.error(`IPC: Failed to unmount vault ${vaultId} in bulk operation`, error);
        }
      }
      
      const successCount = results.filter(r => r.success).length;
      this.logger.info(`IPC: Bulk unmount completed: ${successCount}/${vaultIds.length} successful`);
      
      return {
        success: true,
        data: results,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to perform bulk unmount';
      this.logger.error('IPC: Failed to perform bulk unmount', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle validate path request
   */
  private async handleValidatePath(
    event: IpcMainInvokeEvent,
    path: string
  ): Promise<ApiResponse<boolean>> {
    try {
      this.logger.debug(`IPC: Validating path: ${path}`);
      
      const isValid = await this.serviceCommunicator.validatePath(path);
      
      return {
        success: true,
        data: isValid,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to validate path';
      this.logger.error(`IPC: Failed to validate path: ${path}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle validate sequence request
   */
  private async handleValidateSequence(
    event: IpcMainInvokeEvent,
    sequence: string,
    excludeVaultId?: string
  ): Promise<ApiResponse<boolean>> {
    try {
      this.logger.debug(`IPC: Validating keyboard sequence`);
      
      const isValid = await this.serviceCommunicator.validateSequence(sequence, excludeVaultId);
      
      return {
        success: true,
        data: isValid,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to validate sequence';
      this.logger.error('IPC: Failed to validate keyboard sequence', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Validate vault configuration
   */
  private async validateVaultConfig(config: VaultConfig): Promise<{ isValid: boolean; errors: string[] }> {
    const errors: string[] = [];

    // Validate name
    if (!config.name || config.name.trim().length === 0) {
      errors.push('Vault name is required');
    }

    // Validate path
    if (!config.path || config.path.trim().length === 0) {
      errors.push('Vault path is required');
    } else {
      try {
        const isValidPath = await this.serviceCommunicator.validatePath(config.path);
        if (!isValidPath) {
          errors.push('Vault path is not accessible or writable');
        }
      } catch (error) {
        errors.push('Failed to validate vault path');
      }
    }

    // Validate password
    if (!config.password || config.password.length < 8) {
      errors.push('Password must be at least 8 characters long');
    }

    // Validate keyboard sequence
    if (!config.keyboardSequence || config.keyboardSequence.trim().length === 0) {
      errors.push('Keyboard sequence is required');
    } else {
      try {
        const isValidSequence = await this.serviceCommunicator.validateSequence(config.keyboardSequence);
        if (!isValidSequence) {
          errors.push('Keyboard sequence is already in use');
        }
      } catch (error) {
        errors.push('Failed to validate keyboard sequence');
      }
    }

    return {
      isValid: errors.length === 0,
      errors,
    };
  }

  // ==================== VALIDATION METHODS ====================

  /**
   * Validate create vault arguments
   */
  private validateCreateVaultArgs(config: VaultConfig): void {
    if (!config || typeof config !== 'object') {
      throw new Error('Vault configuration is required');
    }
    
    if (!config.name || typeof config.name !== 'string' || config.name.trim().length === 0) {
      throw new Error('Vault name is required and must be a non-empty string');
    }
    
    if (!config.path || typeof config.path !== 'string' || config.path.trim().length === 0) {
      throw new Error('Vault path is required and must be a non-empty string');
    }
    
    if (!config.password || typeof config.password !== 'string' || config.password.length < 8) {
      throw new Error('Vault password is required and must be at least 8 characters long');
    }
    
    if (!config.keyboardSequence || typeof config.keyboardSequence !== 'string' || config.keyboardSequence.trim().length === 0) {
      throw new Error('Keyboard sequence is required and must be a non-empty string');
    }
  }

  /**
   * Validate update vault arguments
   */
  private validateUpdateVaultArgs(vaultId: string, config: VaultConfig): void {
    this.validateVaultIdArg(vaultId);
    this.validateCreateVaultArgs(config);
  }

  /**
   * Validate vault ID argument
   */
  private validateVaultIdArg(vaultId: string): void {
    if (!vaultId || typeof vaultId !== 'string' || vaultId.trim().length === 0) {
      throw new Error('Vault ID is required and must be a non-empty string');
    }
  }

  /**
   * Validate mount vault arguments
   */
  private validateMountVaultArgs(vaultId: string, password: string): void {
    this.validateVaultIdArg(vaultId);
    
    if (!password || typeof password !== 'string') {
      throw new Error('Password is required and must be a string');
    }
  }

  /**
   * Validate unmount vault arguments
   */
  private validateUnmountVaultArgs(vaultId: string, force?: boolean): void {
    this.validateVaultIdArg(vaultId);
    
    if (force !== undefined && typeof force !== 'boolean') {
      throw new Error('Force parameter must be a boolean');
    }
  }

  /**
   * Validate bulk mount arguments
   */
  private validateBulkMountArgs(vaultIds: string[], passwords: Record<string, string>): void {
    if (!Array.isArray(vaultIds) || vaultIds.length === 0) {
      throw new Error('Vault IDs array is required and must not be empty');
    }
    
    if (!passwords || typeof passwords !== 'object') {
      throw new Error('Passwords object is required');
    }
    
    vaultIds.forEach(id => this.validateVaultIdArg(id));
  }

  /**
   * Validate bulk unmount arguments
   */
  private validateBulkUnmountArgs(vaultIds: string[], force?: boolean): void {
    if (!Array.isArray(vaultIds) || vaultIds.length === 0) {
      throw new Error('Vault IDs array is required and must not be empty');
    }
    
    if (force !== undefined && typeof force !== 'boolean') {
      throw new Error('Force parameter must be a boolean');
    }
    
    vaultIds.forEach(id => this.validateVaultIdArg(id));
  }

  /**
   * Validate path argument
   */
  private validatePathArg(path: string): void {
    if (!path || typeof path !== 'string' || path.trim().length === 0) {
      throw new Error('Path is required and must be a non-empty string');
    }
  }

  /**
   * Validate sequence arguments
   */
  private validateSequenceArgs(sequence: string, excludeVaultId?: string): void {
    if (!sequence || typeof sequence !== 'string' || sequence.trim().length === 0) {
      throw new Error('Keyboard sequence is required and must be a non-empty string');
    }
    
    if (excludeVaultId !== undefined && (typeof excludeVaultId !== 'string' || excludeVaultId.trim().length === 0)) {
      throw new Error('Exclude vault ID must be a non-empty string if provided');
    }
  }

  /**
   * Get error statistics
   */
  public getErrorStats() {
    return this.errorHandler.getErrorStats();
  }

  /**
   * Cleanup handlers
   */
  public cleanup(): void {
    ipcMain.removeHandler('vault:list');
    ipcMain.removeHandler('vault:create');
    ipcMain.removeHandler('vault:update');
    ipcMain.removeHandler('vault:delete');
    ipcMain.removeHandler('vault:mount');
    ipcMain.removeHandler('vault:unmount');
    ipcMain.removeHandler('vault:status');
    ipcMain.removeHandler('vault:bulk-mount');
    ipcMain.removeHandler('vault:bulk-unmount');
    ipcMain.removeHandler('vault:validate-path');
    ipcMain.removeHandler('vault:validate-sequence');
  }
}