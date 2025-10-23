/**
 * Logger Utility
 * 
 * Centralized logging for the Electron main process
 */

import * as fs from 'fs';
import * as path from 'path';
import { app } from 'electron';

export type LogLevel = 'debug' | 'info' | 'warn' | 'error';

export interface LogEntry {
  timestamp: Date;
  level: LogLevel;
  message: string;
  data?: any;
  source: string;
}

export class Logger {
  private logLevel: LogLevel;
  private logFile: string;
  private maxLogSize: number = 10 * 1024 * 1024; // 10MB
  private maxLogFiles: number = 5;

  constructor(logLevel: LogLevel = 'info', logFile?: string) {
    this.logLevel = logLevel;
    this.logFile = logFile || this.getDefaultLogFile();
    this.ensureLogDirectory();
  }

  /**
   * Get default log file path
   */
  private getDefaultLogFile(): string {
    const userDataPath = app.getPath('userData');
    const logsDir = path.join(userDataPath, 'logs');
    return path.join(logsDir, 'main.log');
  }

  /**
   * Ensure log directory exists
   */
  private ensureLogDirectory(): void {
    const logDir = path.dirname(this.logFile);
    if (!fs.existsSync(logDir)) {
      fs.mkdirSync(logDir, { recursive: true });
    }
  }

  /**
   * Check if log level should be logged
   */
  private shouldLog(level: LogLevel): boolean {
    const levels: LogLevel[] = ['debug', 'info', 'warn', 'error'];
    const currentLevelIndex = levels.indexOf(this.logLevel);
    const messageLevelIndex = levels.indexOf(level);
    return messageLevelIndex >= currentLevelIndex;
  }

  /**
   * Format log entry
   */
  private formatLogEntry(entry: LogEntry): string {
    const timestamp = entry.timestamp.toISOString();
    const level = entry.level.toUpperCase().padEnd(5);
    const source = entry.source.padEnd(12);
    
    let message = `[${timestamp}] ${level} ${source} ${entry.message}`;
    
    if (entry.data) {
      try {
        const dataStr = typeof entry.data === 'string' 
          ? entry.data 
          : JSON.stringify(entry.data, null, 2);
        message += `\n${dataStr}`;
      } catch (error) {
        message += `\n[Failed to serialize data: ${error}]`;
      }
    }
    
    return message;
  }

  /**
   * Write log entry to file
   */
  private writeToFile(entry: LogEntry): void {
    try {
      const logMessage = this.formatLogEntry(entry) + '\n';
      
      // Check if log rotation is needed
      if (fs.existsSync(this.logFile)) {
        const stats = fs.statSync(this.logFile);
        if (stats.size > this.maxLogSize) {
          this.rotateLogFiles();
        }
      }
      
      fs.appendFileSync(this.logFile, logMessage, 'utf8');
    } catch (error) {
      console.error('Failed to write to log file:', error);
    }
  }

  /**
   * Rotate log files
   */
  private rotateLogFiles(): void {
    try {
      const logDir = path.dirname(this.logFile);
      const logName = path.basename(this.logFile, '.log');
      
      // Remove oldest log file
      const oldestLog = path.join(logDir, `${logName}.${this.maxLogFiles}.log`);
      if (fs.existsSync(oldestLog)) {
        fs.unlinkSync(oldestLog);
      }
      
      // Rotate existing log files
      for (let i = this.maxLogFiles - 1; i >= 1; i--) {
        const currentLog = path.join(logDir, `${logName}.${i}.log`);
        const nextLog = path.join(logDir, `${logName}.${i + 1}.log`);
        
        if (fs.existsSync(currentLog)) {
          fs.renameSync(currentLog, nextLog);
        }
      }
      
      // Move current log to .1
      const firstRotatedLog = path.join(logDir, `${logName}.1.log`);
      if (fs.existsSync(this.logFile)) {
        fs.renameSync(this.logFile, firstRotatedLog);
      }
    } catch (error) {
      console.error('Failed to rotate log files:', error);
    }
  }

  /**
   * Log a message
   */
  private log(level: LogLevel, message: string, data?: any, source: string = 'main'): void {
    if (!this.shouldLog(level)) {
      return;
    }

    const entry: LogEntry = {
      timestamp: new Date(),
      level,
      message,
      data,
      source,
    };

    // Write to console
    const consoleMessage = this.formatLogEntry(entry);
    switch (level) {
      case 'debug':
        console.debug(consoleMessage);
        break;
      case 'info':
        console.info(consoleMessage);
        break;
      case 'warn':
        console.warn(consoleMessage);
        break;
      case 'error':
        console.error(consoleMessage);
        break;
    }

    // Write to file
    this.writeToFile(entry);
  }

  /**
   * Log debug message
   */
  public debug(message: string, data?: any, source?: string): void {
    this.log('debug', message, data, source);
  }

  /**
   * Log info message
   */
  public info(message: string, data?: any, source?: string): void {
    this.log('info', message, data, source);
  }

  /**
   * Log warning message
   */
  public warn(message: string, data?: any, source?: string): void {
    this.log('warn', message, data, source);
  }

  /**
   * Log error message
   */
  public error(message: string, error?: any, source?: string): void {
    let errorData = error;
    
    if (error instanceof Error) {
      errorData = {
        name: error.name,
        message: error.message,
        stack: error.stack,
      };
    }
    
    this.log('error', message, errorData, source);
  }

  /**
   * Set log level
   */
  public setLogLevel(level: LogLevel): void {
    this.logLevel = level;
    this.info(`Log level changed to: ${level}`);
  }

  /**
   * Get current log level
   */
  public getLogLevel(): LogLevel {
    return this.logLevel;
  }

  /**
   * Get log file path
   */
  public getLogFile(): string {
    return this.logFile;
  }

  /**
   * Clear log file
   */
  public clearLog(): void {
    try {
      if (fs.existsSync(this.logFile)) {
        fs.unlinkSync(this.logFile);
      }
      this.info('Log file cleared');
    } catch (error) {
      this.error('Failed to clear log file', error);
    }
  }

  /**
   * Get recent log entries
   */
  public getRecentLogs(lines: number = 100): string[] {
    try {
      if (!fs.existsSync(this.logFile)) {
        return [];
      }
      
      const content = fs.readFileSync(this.logFile, 'utf8');
      const allLines = content.split('\n').filter(line => line.trim());
      
      return allLines.slice(-lines);
    } catch (error) {
      this.error('Failed to read log file', error);
      return [];
    }
  }
}