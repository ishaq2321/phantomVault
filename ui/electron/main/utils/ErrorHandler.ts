/**
 * Error Handler
 * 
 * Centralized error handling for IPC communication and service operations
 */

import { Logger } from './Logger';

export enum ErrorType {
  IPC_ERROR = 'IPC_ERROR',
  SERVICE_ERROR = 'SERVICE_ERROR',
  VALIDATION_ERROR = 'VALIDATION_ERROR',
  FILESYSTEM_ERROR = 'FILESYSTEM_ERROR',
  NETWORK_ERROR = 'NETWORK_ERROR',
  TIMEOUT_ERROR = 'TIMEOUT_ERROR',
  AUTHENTICATION_ERROR = 'AUTHENTICATION_ERROR',
  PERMISSION_ERROR = 'PERMISSION_ERROR',
  UNKNOWN_ERROR = 'UNKNOWN_ERROR',
}

export enum ErrorSeverity {
  LOW = 'LOW',
  MEDIUM = 'MEDIUM',
  HIGH = 'HIGH',
  CRITICAL = 'CRITICAL',
}

export interface ErrorContext {
  operation: string;
  component: string;
  userId?: string;
  vaultId?: string;
  additionalData?: Record<string, any>;
}

export interface ProcessedError {
  type: ErrorType;
  severity: ErrorSeverity;
  message: string;
  userMessage: string;
  code: string;
  context: ErrorContext;
  timestamp: Date;
  stack?: string;
  retryable: boolean;
  suggestedActions: string[];
}

export class ErrorHandler {
  private logger: Logger;
  private errorCounts: Map<string, number> = new Map();
  private errorHistory: ProcessedError[] = [];
  private maxHistorySize: number = 1000;

  constructor(logger: Logger) {
    this.logger = logger;
  }

  /**
   * Process and handle an error
   */
  public handleError(error: any, context: ErrorContext): ProcessedError {
    const processedError = this.processError(error, context);
    
    // Log the error
    this.logError(processedError);
    
    // Track error frequency
    this.trackError(processedError);
    
    // Store in history
    this.storeError(processedError);
    
    return processedError;
  }

  /**
   * Process raw error into structured format
   */
  private processError(error: any, context: ErrorContext): ProcessedError {
    const timestamp = new Date();
    let type = ErrorType.UNKNOWN_ERROR;
    let severity = ErrorSeverity.MEDIUM;
    let message = 'An unknown error occurred';
    let userMessage = 'Something went wrong. Please try again.';
    let code = 'UNKNOWN_ERROR';
    let retryable = false;
    let suggestedActions: string[] = [];
    let stack: string | undefined;

    // Extract error information
    if (error instanceof Error) {
      message = error.message;
      stack = error.stack;
    } else if (typeof error === 'string') {
      message = error;
    } else if (error && typeof error === 'object') {
      message = error.message || error.toString();
      code = error.code || code;
      type = error.type || type;
    }

    // Classify error type and determine appropriate handling
    const classification = this.classifyError(message, code, context);
    type = classification.type;
    severity = classification.severity;
    userMessage = classification.userMessage;
    retryable = classification.retryable;
    suggestedActions = classification.suggestedActions;

    return {
      type,
      severity,
      message,
      userMessage,
      code,
      context,
      timestamp,
      stack,
      retryable,
      suggestedActions,
    };
  }

  /**
   * Classify error based on message, code, and context
   */
  private classifyError(message: string, code: string, context: ErrorContext): {
    type: ErrorType;
    severity: ErrorSeverity;
    userMessage: string;
    retryable: boolean;
    suggestedActions: string[];
  } {
    const lowerMessage = message.toLowerCase();
    const lowerCode = code.toLowerCase();

    // IPC Communication Errors
    if (lowerMessage.includes('ipc') || lowerMessage.includes('invoke') || lowerMessage.includes('channel')) {
      return {
        type: ErrorType.IPC_ERROR,
        severity: ErrorSeverity.HIGH,
        userMessage: 'Communication error with the application service. Please restart the application.',
        retryable: true,
        suggestedActions: [
          'Restart the application',
          'Check if the service is running',
          'Contact support if the problem persists',
        ],
      };
    }

    // Service Connection Errors
    if (lowerMessage.includes('service') || lowerMessage.includes('connection') || lowerMessage.includes('timeout')) {
      return {
        type: ErrorType.SERVICE_ERROR,
        severity: ErrorSeverity.HIGH,
        userMessage: 'Unable to connect to the PhantomVault service. Please check your installation.',
        retryable: true,
        suggestedActions: [
          'Check if PhantomVault service is running',
          'Restart the application',
          'Reinstall PhantomVault if the problem persists',
        ],
      };
    }

    // Authentication Errors
    if (lowerMessage.includes('password') || lowerMessage.includes('auth') || lowerMessage.includes('credential')) {
      return {
        type: ErrorType.AUTHENTICATION_ERROR,
        severity: ErrorSeverity.MEDIUM,
        userMessage: 'Authentication failed. Please check your password and try again.',
        retryable: true,
        suggestedActions: [
          'Verify your password is correct',
          'Check if Caps Lock is enabled',
          'Reset your password if necessary',
        ],
      };
    }

    // File System Errors
    if (lowerMessage.includes('file') || lowerMessage.includes('directory') || lowerMessage.includes('path') || 
        lowerMessage.includes('enoent') || lowerMessage.includes('eacces') || lowerMessage.includes('eperm')) {
      return {
        type: ErrorType.FILESYSTEM_ERROR,
        severity: ErrorSeverity.MEDIUM,
        userMessage: 'File system error. Please check the file path and permissions.',
        retryable: true,
        suggestedActions: [
          'Check if the file or directory exists',
          'Verify you have the necessary permissions',
          'Try selecting a different location',
        ],
      };
    }

    // Permission Errors
    if (lowerMessage.includes('permission') || lowerMessage.includes('access denied') || lowerMessage.includes('forbidden')) {
      return {
        type: ErrorType.PERMISSION_ERROR,
        severity: ErrorSeverity.MEDIUM,
        userMessage: 'Permission denied. Please check your access rights.',
        retryable: false,
        suggestedActions: [
          'Run the application as administrator',
          'Check file and folder permissions',
          'Contact your system administrator',
        ],
      };
    }

    // Validation Errors
    if (lowerMessage.includes('invalid') || lowerMessage.includes('validation') || lowerMessage.includes('required')) {
      return {
        type: ErrorType.VALIDATION_ERROR,
        severity: ErrorSeverity.LOW,
        userMessage: 'Invalid input. Please check your entries and try again.',
        retryable: true,
        suggestedActions: [
          'Check all required fields are filled',
          'Verify input format is correct',
          'Review validation messages',
        ],
      };
    }

    // Network Errors
    if (lowerMessage.includes('network') || lowerMessage.includes('dns') || lowerMessage.includes('host')) {
      return {
        type: ErrorType.NETWORK_ERROR,
        severity: ErrorSeverity.MEDIUM,
        userMessage: 'Network error. Please check your internet connection.',
        retryable: true,
        suggestedActions: [
          'Check your internet connection',
          'Try again in a few moments',
          'Contact your network administrator',
        ],
      };
    }

    // Timeout Errors
    if (lowerMessage.includes('timeout') || lowerMessage.includes('timed out')) {
      return {
        type: ErrorType.TIMEOUT_ERROR,
        severity: ErrorSeverity.MEDIUM,
        userMessage: 'Operation timed out. Please try again.',
        retryable: true,
        suggestedActions: [
          'Try the operation again',
          'Check your network connection',
          'Contact support if timeouts persist',
        ],
      };
    }

    // Default classification
    return {
      type: ErrorType.UNKNOWN_ERROR,
      severity: ErrorSeverity.MEDIUM,
      userMessage: 'An unexpected error occurred. Please try again.',
      retryable: true,
      suggestedActions: [
        'Try the operation again',
        'Restart the application',
        'Contact support if the problem persists',
      ],
    };
  }

  /**
   * Log error with appropriate level
   */
  private logError(error: ProcessedError): void {
    const logMessage = `${error.type}: ${error.message}`;
    const logData = {
      code: error.code,
      context: error.context,
      userMessage: error.userMessage,
      retryable: error.retryable,
      suggestedActions: error.suggestedActions,
    };

    switch (error.severity) {
      case ErrorSeverity.CRITICAL:
        this.logger.error(logMessage, { ...logData, stack: error.stack });
        break;
      case ErrorSeverity.HIGH:
        this.logger.error(logMessage, logData);
        break;
      case ErrorSeverity.MEDIUM:
        this.logger.warn(logMessage, logData);
        break;
      case ErrorSeverity.LOW:
        this.logger.info(logMessage, logData);
        break;
    }
  }

  /**
   * Track error frequency
   */
  private trackError(error: ProcessedError): void {
    const errorKey = `${error.type}:${error.code}:${error.context.operation}`;
    const currentCount = this.errorCounts.get(errorKey) || 0;
    this.errorCounts.set(errorKey, currentCount + 1);

    // Log if error is becoming frequent
    if (currentCount > 5) {
      this.logger.warn(`Frequent error detected: ${errorKey} (${currentCount + 1} occurrences)`);
    }
  }

  /**
   * Store error in history
   */
  private storeError(error: ProcessedError): void {
    this.errorHistory.unshift(error);
    
    // Limit history size
    if (this.errorHistory.length > this.maxHistorySize) {
      this.errorHistory = this.errorHistory.slice(0, this.maxHistorySize);
    }
  }

  /**
   * Get error statistics
   */
  public getErrorStats(): {
    totalErrors: number;
    errorsByType: Record<ErrorType, number>;
    errorsBySeverity: Record<ErrorSeverity, number>;
    frequentErrors: Array<{ key: string; count: number }>;
    recentErrors: ProcessedError[];
  } {
    const errorsByType = {} as Record<ErrorType, number>;
    const errorsBySeverity = {} as Record<ErrorSeverity, number>;

    // Initialize counters
    Object.values(ErrorType).forEach(type => {
      errorsByType[type] = 0;
    });
    Object.values(ErrorSeverity).forEach(severity => {
      errorsBySeverity[severity] = 0;
    });

    // Count errors
    this.errorHistory.forEach(error => {
      errorsByType[error.type]++;
      errorsBySeverity[error.severity]++;
    });

    // Get frequent errors
    const frequentErrors = Array.from(this.errorCounts.entries())
      .map(([key, count]) => ({ key, count }))
      .sort((a, b) => b.count - a.count)
      .slice(0, 10);

    return {
      totalErrors: this.errorHistory.length,
      errorsByType,
      errorsBySeverity,
      frequentErrors,
      recentErrors: this.errorHistory.slice(0, 20),
    };
  }

  /**
   * Clear error history
   */
  public clearHistory(): void {
    this.errorHistory = [];
    this.errorCounts.clear();
    this.logger.info('Error history cleared');
  }

  /**
   * Get user-friendly error message
   */
  public getUserMessage(error: any, context: ErrorContext): string {
    const processedError = this.processError(error, context);
    return processedError.userMessage;
  }

  /**
   * Check if error is retryable
   */
  public isRetryable(error: any, context: ErrorContext): boolean {
    const processedError = this.processError(error, context);
    return processedError.retryable;
  }

  /**
   * Get suggested actions for error
   */
  public getSuggestedActions(error: any, context: ErrorContext): string[] {
    const processedError = this.processError(error, context);
    return processedError.suggestedActions;
  }

  /**
   * Create error response for IPC
   */
  public createErrorResponse(error: any, context: ErrorContext): {
    success: false;
    error: string;
    code: string;
    retryable: boolean;
    suggestedActions: string[];
    timestamp: Date;
  } {
    const processedError = this.handleError(error, context);
    
    return {
      success: false,
      error: processedError.userMessage,
      code: processedError.code,
      retryable: processedError.retryable,
      suggestedActions: processedError.suggestedActions,
      timestamp: processedError.timestamp,
    };
  }
}