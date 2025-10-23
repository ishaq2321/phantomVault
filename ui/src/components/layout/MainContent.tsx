/**
 * Main Content Component
 * 
 * Content area that renders different views based on current route
 */

import React, { Suspense } from 'react';
import { AppView } from '../../App';

// Import view components
import { VaultDashboard } from '../dashboard/VaultDashboard';
import { VaultManager } from '../vault-manager/VaultManager';
import { EnhancedActivityMonitor } from '../activity-monitor/EnhancedActivityMonitor';
import { Settings } from '../settings/Settings';

export interface MainContentProps {
  currentView: AppView;
  className?: string;
}

/**
 * Loading component for suspense fallback
 */
const LoadingSpinner: React.FC<{ message?: string }> = ({ message = 'Loading...' }) => (
  <div className="loading-container">
    <div className="loading-spinner">
      <div className="spinner-ring"></div>
      <div className="spinner-ring"></div>
      <div className="spinner-ring"></div>
    </div>
    <p className="loading-message">{message}</p>
  </div>
);

/**
 * Error boundary for view components
 */
class ViewErrorBoundary extends React.Component<
  { children: React.ReactNode; viewName: string },
  { hasError: boolean; error: Error | null }
> {
  constructor(props: { children: React.ReactNode; viewName: string }) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error: Error) {
    return { hasError: true, error };
  }

  componentDidCatch(error: Error, errorInfo: React.ErrorInfo) {
    console.error(`Error in ${this.props.viewName} view:`, error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return (
        <div className="error-container">
          <div className="error-content">
            <div className="error-icon">⚠️</div>
            <h2>Something went wrong</h2>
            <p>An error occurred while loading the {this.props.viewName} view.</p>
            <div className="error-details">
              <details>
                <summary>Error Details</summary>
                <pre>{this.state.error?.stack}</pre>
              </details>
            </div>
            <div className="error-actions">
              <button 
                onClick={() => this.setState({ hasError: false, error: null })}
                className="retry-button\"
              >
                Try Again
              </button>
              <button 
                onClick={() => window.location.reload()}
                className="reload-button\"
              >
                Reload Application
              </button>
            </div>
          </div>
        </div>
      );
    }

    return this.props.children;
  }
}

/**
 * Main content component with view routing
 */
export const MainContent: React.FC<MainContentProps> = ({
  currentView,
  className = '',
}) => {
  // Render the appropriate view component
  const renderView = () => {
    switch (currentView) {
      case 'dashboard':
        return (
          <ViewErrorBoundary viewName="Dashboard">
            <Suspense fallback={<LoadingSpinner message="Loading dashboard...\" />}>
              <VaultDashboard />
            </Suspense>
          </ViewErrorBoundary>
        );

      case 'vaults':
        return (
          <ViewErrorBoundary viewName="Vault Manager">
            <Suspense fallback={<LoadingSpinner message="Loading vault manager...\" />}>
              <VaultManager />
            </Suspense>
          </ViewErrorBoundary>
        );

      case 'activity':
        return (
          <ViewErrorBoundary viewName="Activity Monitor">
            <Suspense fallback={<LoadingSpinner message="Loading activity monitor...\" />}>
              <EnhancedActivityMonitor 
                maxEntries={1000}
                autoScroll={true}
                showFilters={true}
                enableRealTime={true}
              />
            </Suspense>
          </ViewErrorBoundary>
        );

      case 'settings':
        return (
          <ViewErrorBoundary viewName="Settings">
            <Suspense fallback={<LoadingSpinner message="Loading settings...\" />}>
              <Settings />
            </Suspense>
          </ViewErrorBoundary>
        );

      default:
        return (
          <div className="unknown-view">
            <div className="unknown-view-content">
              <div className="unknown-view-icon">❓</div>
              <h2>Unknown View</h2>
              <p>The requested view '{currentView}' could not be found.</p>
              <button 
                onClick={() => window.location.reload()}
                className="back-button\"
              >
                Return to Dashboard
              </button>
            </div>
          </div>
        );
    }
  };

  return (
    <main className={`main-content ${className}`}>
      <div className="content-container">
        {renderView()}
      </div>
    </main>
  );
};