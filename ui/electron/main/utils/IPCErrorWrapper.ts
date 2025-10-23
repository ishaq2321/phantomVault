/**
 * IPC Error Wrapper
 * 
 * Wrapper for IPC handlers that provides consistent error handling and response formatting
 */

import { IpcMainInvokeEvent } from 'electron';
import { ApiResponse } from '../../../src/types';
import { ErrorHandler, ErrorContext } from './ErrorHandler';
import { Logger } from './Logger';

export type IPCHandler<T = any, Args extends any[] = any[]> = (
  event: IpcMainInvokeEvent,
  ...args: Args
) => Promise<T>;

export class IPCErrorWrapper {
  private errorHandler: ErrorHandler;
  private logger: Logger;

  constructor(errorHandler: ErrorHandler, logger: Logger) {
    this.errorHandler = errorHandler;
    this.logger = logger;
  }

  /**
   * Wrap an IPC handler with error handling
   */
  public wrapHandler<T, Args extends any[]>(
    handler: IPCHandler<T, Args>,
    context: Omit<ErrorContext, 'userId'>
  ): IPCHandler<ApiResponse<T>, Args> {
    return async (event: IpcMainInvokeEvent, ...args: Args): Promise<ApiResponse<T>> => {
      const startTime = Date.now();
      const fullContext: ErrorContext = {
        ...context,
        userId: this.extractUserId(event),
      };

      try {
        this.logger.debug(`IPC: Starting ${context.operation}`, { args });

        const result = await handler(event, ...args);
        const duration = Date.now() - startTime;

        this.logger.debug(`IPC: Completed ${context.operation} in ${duration}ms`);

        return {
          success: true,
          data: result,
          timestamp: new Date(),
        };
      } catch (error) {
        const duration = Date.now() - startTime;
        this.logger.error(`IPC: Failed ${context.operation} after ${duration}ms`, error);

        return this.errorHandler.createErrorResponse(error, fullContext) as ApiResponse<T>;
      }
    };
  }

  /**
   * Wrap a void IPC handler with error handling
   */
  public wrapVoidHandler<Args extends any[]>(
    handler: IPCHandler<void, Args>,
    context: Omit<ErrorContext, 'userId'>
  ): IPCHandler<ApiResponse, Args> {
    return async (event: IpcMainInvokeEvent, ...args: Args): Promise<ApiResponse> => {
      const startTime = Date.now();
      const fullContext: ErrorContext = {
        ...context,
        userId: this.extractUserId(event),
      };

      try {
        this.logger.debug(`IPC: Starting ${context.operation}`, { args });

        await handler(event, ...args);
        const duration = Date.now() - startTime;

        this.logger.debug(`IPC: Completed ${context.operation} in ${duration}ms`);

        return {
          success: true,
          timestamp: new Date(),
        };
      } catch (error) {
        const duration = Date.now() - startTime;
        this.logger.error(`IPC: Failed ${context.operation} after ${duration}ms`, error);

        return this.errorHandler.createErrorResponse(error, fullContext);
      }
    };
  }

  /**
   * Create a timeout wrapper for handlers
   */
  public withTimeout<T, Args extends any[]>(
    handler: IPCHandler<T, Args>,
    timeoutMs: number = 30000
  ): IPCHandler<T, Args> {
    return async (event: IpcMainInvokeEvent, ...args: Args): Promise<T> => {
      return Promise.race([
        handler(event, ...args),
        new Promise<never>((_, reject) => {
          setTimeout(() => {
            reject(new Error(`Operation timed out after ${timeoutMs}ms`));
          }, timeoutMs);
        }),
      ]);
    };
  }

  /**
   * Create a retry wrapper for handlers
   */
  public withRetry<T, Args extends any[]>(
    handler: IPCHandler<T, Args>,
    maxRetries: number = 3,
    retryDelay: number = 1000
  ): IPCHandler<T, Args> {
    return async (event: IpcMainInvokeEvent, ...args: Args): Promise<T> => {
      let lastError: any;

      for (let attempt = 1; attempt <= maxRetries; attempt++) {
        try {
          return await handler(event, ...args);
        } catch (error) {
          lastError = error;
          
          // Don't retry on the last attempt
          if (attempt === maxRetries) {
            break;
          }

          // Check if error is retryable
          const context: ErrorContext = {
            operation: 'retry-check',
            component: 'ipc-wrapper',
          };
          
          if (!this.errorHandler.isRetryable(error, context)) {
            break;
          }

          this.logger.warn(`IPC: Retry attempt ${attempt}/${maxRetries} failed, retrying in ${retryDelay}ms`, error);
          
          // Wait before retrying
          await new Promise(resolve => setTimeout(resolve, retryDelay));
        }
      }

      throw lastError;
    };
  }

  /**
   * Create a validation wrapper for handlers
   */
  public withValidation<T, Args extends any[]>(
    handler: IPCHandler<T, Args>,
    validator: (...args: Args) => void | Promise<void>
  ): IPCHandler<T, Args> {
    return async (event: IpcMainInvokeEvent, ...args: Args): Promise<T> => {
      // Validate arguments
      await validator(...args);
      
      // Call original handler
      return handler(event, ...args);
    };
  }

  /**
   * Create a rate-limited wrapper for handlers
   */
  public withRateLimit<T, Args extends any[]>(
    handler: IPCHandler<T, Args>,
    maxCalls: number = 10,
    windowMs: number = 60000
  ): IPCHandler<T, Args> {
    const callHistory = new Map<string, number[]>();

    return async (event: IpcMainInvokeEvent, ...args: Args): Promise<T> => {
      const userId = this.extractUserId(event);
      const now = Date.now();
      
      // Get call history for this user
      const userCalls = callHistory.get(userId) || [];
      
      // Remove old calls outside the window
      const recentCalls = userCalls.filter(callTime => now - callTime < windowMs);
      
      // Check rate limit
      if (recentCalls.length >= maxCalls) {
        throw new Error(`Rate limit exceeded: ${maxCalls} calls per ${windowMs}ms`);
      }
      
      // Record this call
      recentCalls.push(now);
      callHistory.set(userId, recentCalls);
      
      // Call original handler
      return handler(event, ...args);
    };
  }

  /**
   * Extract user ID from IPC event (for logging and rate limiting)
   */
  private extractUserId(event: IpcMainInvokeEvent): string {
    // In a real implementation, this would extract the actual user ID
    // For now, use the process ID as a unique identifier
    return `process-${event.processId}`;
  }

  /**
   * Create a comprehensive wrapper with multiple features
   */
  public createSecureHandler<T, Args extends any[]>(
    handler: IPCHandler<T, Args>,
    options: {
      context: Omit<ErrorContext, 'userId'>;
      timeout?: number;
      maxRetries?: number;
      retryDelay?: number;
      validator?: (...args: Args) => void | Promise<void>;
      rateLimit?: { maxCalls: number; windowMs: number };
    }
  ): IPCHandler<ApiResponse<T>, Args> {
    let wrappedHandler = handler;

    // Apply validation if provided
    if (options.validator) {
      wrappedHandler = this.withValidation(wrappedHandler, options.validator);
    }

    // Apply rate limiting if provided
    if (options.rateLimit) {
      wrappedHandler = this.withRateLimit(
        wrappedHandler,
        options.rateLimit.maxCalls,
        options.rateLimit.windowMs
      );
    }

    // Apply retry logic if provided
    if (options.maxRetries && options.maxRetries > 1) {
      wrappedHandler = this.withRetry(
        wrappedHandler,
        options.maxRetries,
        options.retryDelay
      );
    }

    // Apply timeout if provided
    if (options.timeout) {
      wrappedHandler = this.withTimeout(wrappedHandler, options.timeout);
    }

    // Apply error handling wrapper
    return this.wrapHandler(wrappedHandler, options.context);
  }
}