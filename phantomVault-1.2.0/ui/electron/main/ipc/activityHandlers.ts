/**
 * Activity IPC Handlers
 * 
 * IPC handlers for activity monitoring and logging
 */

import { ipcMain, IpcMainInvokeEvent, WebContents } from 'electron';
import { ActivityLogEntry, ActivityFilter, ApiResponse } from '../../../src/types';
import { ServiceCommunicator } from '../services/ServiceCommunicator';
import { Logger } from '../utils/Logger';

export class ActivityHandlers {
  private serviceCommunicator: ServiceCommunicator;
  private logger: Logger;
  private subscribers: Set<WebContents> = new Set();

  constructor(serviceCommunicator: ServiceCommunicator, logger: Logger) {
    this.serviceCommunicator = serviceCommunicator;
    this.logger = logger;
    this.registerHandlers();
    this.setupServiceListeners();
  }

  /**
   * Register all activity-related IPC handlers
   */
  private registerHandlers(): void {
    ipcMain.handle('activity:get-log', this.handleGetActivityLog.bind(this));
    ipcMain.handle('activity:clear-log', this.handleClearActivityLog.bind(this));
    ipcMain.handle('activity:subscribe', this.handleSubscribeToActivity.bind(this));
    ipcMain.handle('activity:unsubscribe', this.handleUnsubscribeFromActivity.bind(this));
  }

  /**
   * Set up service event listeners
   */
  private setupServiceListeners(): void {
    this.serviceCommunicator.on('activity-log-entry', (entry: ActivityLogEntry) => {
      this.broadcastActivityEntry(entry);
    });
  }

  /**
   * Handle get activity log request
   */
  private async handleGetActivityLog(
    event: IpcMainInvokeEvent,
    filters?: Partial<ActivityFilter>
  ): Promise<ApiResponse<ActivityLogEntry[]>> {
    try {
      this.logger.debug('IPC: Getting activity log', filters);
      
      const entries = await this.serviceCommunicator.getActivityLog(filters);
      
      this.logger.debug(`IPC: Retrieved ${entries.length} activity log entries`);
      return {
        success: true,
        data: entries,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to get activity log';
      this.logger.error('IPC: Failed to get activity log', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle clear activity log request
   */
  private async handleClearActivityLog(event: IpcMainInvokeEvent): Promise<ApiResponse> {
    try {
      this.logger.info('IPC: Clearing activity log');
      
      await this.serviceCommunicator.clearActivityLog();
      
      this.logger.info('IPC: Successfully cleared activity log');
      return {
        success: true,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to clear activity log';
      this.logger.error('IPC: Failed to clear activity log', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle subscribe to activity updates
   */
  private async handleSubscribeToActivity(event: IpcMainInvokeEvent): Promise<ApiResponse<boolean>> {
    try {
      this.logger.debug('IPC: Subscribing to activity updates');
      
      const webContents = event.sender;
      this.subscribers.add(webContents);
      
      // Clean up when renderer is destroyed
      webContents.once('destroyed', () => {
        this.subscribers.delete(webContents);
        this.logger.debug('IPC: Cleaned up activity subscription for destroyed renderer');
      });
      
      this.logger.debug(`IPC: Activity subscription added (${this.subscribers.size} total subscribers)`);
      return {
        success: true,
        data: true,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to subscribe to activity';
      this.logger.error('IPC: Failed to subscribe to activity updates', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle unsubscribe from activity updates
   */
  private async handleUnsubscribeFromActivity(event: IpcMainInvokeEvent): Promise<ApiResponse<boolean>> {
    try {
      this.logger.debug('IPC: Unsubscribing from activity updates');
      
      const webContents = event.sender;
      const wasSubscribed = this.subscribers.delete(webContents);
      
      this.logger.debug(`IPC: Activity subscription removed (${this.subscribers.size} total subscribers)`);
      return {
        success: true,
        data: wasSubscribed,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to unsubscribe from activity';
      this.logger.error('IPC: Failed to unsubscribe from activity updates', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Broadcast activity entry to all subscribers
   */
  private broadcastActivityEntry(entry: ActivityLogEntry): void {
    if (this.subscribers.size === 0) {
      return;
    }

    this.logger.debug(`Broadcasting activity entry to ${this.subscribers.size} subscribers`);
    
    // Remove destroyed web contents
    const validSubscribers = new Set<WebContents>();
    
    for (const webContents of this.subscribers) {
      if (!webContents.isDestroyed()) {
        validSubscribers.add(webContents);
        
        try {
          webContents.send('activity:new-entry', entry);
        } catch (error) {
          this.logger.warn('Failed to send activity entry to subscriber', error);
        }
      }
    }
    
    // Update subscribers set
    this.subscribers = validSubscribers;
  }

  /**
   * Get subscriber count
   */
  public getSubscriberCount(): number {
    return this.subscribers.size;
  }

  /**
   * Cleanup handlers
   */
  public cleanup(): void {
    ipcMain.removeHandler('activity:get-log');
    ipcMain.removeHandler('activity:clear-log');
    ipcMain.removeHandler('activity:subscribe');
    ipcMain.removeHandler('activity:unsubscribe');
    
    this.subscribers.clear();
  }
}