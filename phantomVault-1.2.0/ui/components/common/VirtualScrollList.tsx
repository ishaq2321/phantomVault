/**
 * Virtual Scroll List Component
 * 
 * High-performance virtual scrolling component for large lists
 */

import React, { useState, useEffect, useCallback, useMemo, useRef } from 'react';

export interface VirtualScrollListProps<T> {
  items: T[];
  renderItem: (item: T, index: number) => React.ReactNode;
  itemHeight: number;
  containerHeight: string | number;
  overscan?: number;
  className?: string;
}

/**
 * Virtual scroll list component
 */
export function VirtualScrollList<T>({
  items,
  renderItem,
  itemHeight,
  containerHeight,
  overscan = 5,
  className = '',
}: VirtualScrollListProps<T>) {
  const [scrollTop, setScrollTop] = useState(0);
  const scrollElementRef = useRef<HTMLDivElement>(null);

  // ==================== CALCULATIONS ====================

  const containerHeightPx = useMemo(() => {
    if (typeof containerHeight === 'string') {
      // Parse height string (e.g., "400px" -> 400)
      const match = containerHeight.match(/(\d+)/);
      return match ? parseInt(match[1], 10) : 400;
    }
    return containerHeight;
  }, [containerHeight]);

  const totalHeight = items.length * itemHeight;
  const visibleItemCount = Math.ceil(containerHeightPx / itemHeight);

  const startIndex = useMemo(() => {
    const index = Math.floor(scrollTop / itemHeight);
    return Math.max(0, index - overscan);
  }, [scrollTop, itemHeight, overscan]);

  const endIndex = useMemo(() => {
    const index = startIndex + visibleItemCount + overscan * 2;
    return Math.min(items.length - 1, index);
  }, [startIndex, visibleItemCount, overscan, items.length]);

  const visibleItems = useMemo(() => {
    return items.slice(startIndex, endIndex + 1);
  }, [items, startIndex, endIndex]);

  const offsetY = startIndex * itemHeight;

  // ==================== HANDLERS ====================

  const handleScroll = useCallback((event: React.UIEvent<HTMLDivElement>) => {
    setScrollTop(event.currentTarget.scrollTop);
  }, []);

  // ==================== EFFECTS ====================

  useEffect(() => {
    // Reset scroll position when items change significantly
    if (scrollElementRef.current && items.length === 0) {
      scrollElementRef.current.scrollTop = 0;
      setScrollTop(0);
    }
  }, [items.length]);

  // ==================== RENDER ====================

  if (items.length === 0) {
    return (
      <div 
        className={`virtual-scroll-list empty ${className}`}
        style={{ height: containerHeight }}
      >
        <div className="empty-list-message">
          No items to display
        </div>
      </div>
    );
  }

  return (
    <div
      ref={scrollElementRef}
      className={`virtual-scroll-list ${className}`}
      style={{ 
        height: containerHeight,
        overflow: 'auto',
      }}
      onScroll={handleScroll}
    >
      {/* Total height container to maintain scrollbar */}
      <div style={{ height: totalHeight, position: 'relative' }}>
        {/* Visible items container */}
        <div
          style={{
            transform: `translateY(${offsetY}px)`,
            position: 'absolute',
            top: 0,
            left: 0,
            right: 0,
          }}
        >
          {visibleItems.map((item, index) => {
            const actualIndex = startIndex + index;
            return (
              <div
                key={actualIndex}
                style={{
                  height: itemHeight,
                  overflow: 'hidden',
                }}
              >
                {renderItem(item, actualIndex)}
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}