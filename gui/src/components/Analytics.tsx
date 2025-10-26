/**
 * PhantomVault Analytics Component
 * 
 * Comprehensive analytics dashboard with usage statistics, security events,
 * and access pattern visualization.
 */

import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Card,
  CardContent,
  Grid,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Paper,
  Chip,
  LinearProgress,
  Stack,
  Divider,
  Alert,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Button,
} from '@mui/material';
import {
  TrendingUp as TrendingUpIcon,
  Security as SecurityIcon,
  Folder as FolderIcon,
  AccessTime as TimeIcon,
  Warning as WarningIcon,
  CheckCircle as CheckCircleIcon,
  Error as ErrorIcon,
  Download as DownloadIcon,
} from '@mui/icons-material';

interface UsageStatistics {
  totalProfiles: number;
  totalFolders: number;
  totalUnlockAttempts: number;
  successfulUnlocks: number;
  failedUnlocks: number;
  keyboardSequenceDetections: number;
  securityViolations: number;
  totalUptime: string;
  firstUse: string;
  lastActivity: string;
}

interface SecurityEvent {
  id: string;
  type: string;
  level: 'INFO' | 'WARNING' | 'CRITICAL';
  description: string;
  timestamp: string;
  profileId?: string;
}

interface ActivityData {
  date: string;
  unlocks: number;
  violations: number;
  detections: number;
}

const Analytics: React.FC = () => {
  const [statistics, setStatistics] = useState<UsageStatistics | null>(null);
  const [securityEvents, setSecurityEvents] = useState<SecurityEvent[]>([]);
  const [activityData, setActivityData] = useState<ActivityData[]>([]);
  const [loading, setLoading] = useState(false);
  const [timeRange, setTimeRange] = useState('7d');
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    loadAnalyticsData();
  }, [timeRange]);

  const loadAnalyticsData = async () => {
    try {
      setLoading(true);
      
      // Load analytics data from service
      const response = await window.phantomVault.ipc.getSystemAnalytics(timeRange);
      if (response.success) {
        setStatistics(response.statistics);
      } else {
        setError('Failed to load analytics: ' + response.error);
        return;
      }
      
      // Mock data for events and activity (will be replaced with actual service calls)
      const mockStatistics: UsageStatistics = response.statistics;

      const mockEvents: SecurityEvent[] = [
        {
          id: '1',
          type: 'FOLDER_UNLOCKED_TEMPORARY',
          level: 'INFO',
          description: 'Folder "Documents" unlocked temporarily',
          timestamp: '2024-01-20 14:30:15',
          profileId: 'profile1',
        },
        {
          id: '2',
          type: 'KEYBOARD_SEQUENCE_DETECTED',
          level: 'INFO',
          description: 'Ctrl+Alt+V sequence detected',
          timestamp: '2024-01-20 14:29:45',
        },
        {
          id: '3',
          type: 'PROFILE_AUTH_FAILED',
          level: 'WARNING',
          description: 'Failed authentication attempt for profile "Work"',
          timestamp: '2024-01-20 12:15:30',
          profileId: 'profile2',
        },
        {
          id: '4',
          type: 'SECURITY_VIOLATION',
          level: 'CRITICAL',
          description: 'Multiple failed authentication attempts detected',
          timestamp: '2024-01-19 18:45:22',
        },
      ];

      const mockActivity: ActivityData[] = [
        { date: '2024-01-20', unlocks: 8, violations: 1, detections: 5 },
        { date: '2024-01-19', unlocks: 12, violations: 2, detections: 7 },
        { date: '2024-01-18', unlocks: 6, violations: 0, detections: 3 },
        { date: '2024-01-17', unlocks: 9, violations: 0, detections: 4 },
        { date: '2024-01-16', unlocks: 15, violations: 1, detections: 8 },
        { date: '2024-01-15', unlocks: 4, violations: 0, detections: 2 },
        { date: '2024-01-14', unlocks: 7, violations: 0, detections: 6 },
      ];

      setStatistics(mockStatistics);
      setSecurityEvents(mockEvents);
      setActivityData(mockActivity);
    } catch (err) {
      setError('Failed to load analytics data');
    } finally {
      setLoading(false);
    }
  };

  const getEventIcon = (type: string) => {
    switch (type) {
      case 'SECURITY_VIOLATION':
        return <ErrorIcon color="error" />;
      case 'PROFILE_AUTH_FAILED':
        return <WarningIcon color="warning" />;
      default:
        return <CheckCircleIcon color="success" />;
    }
  };

  const getEventColor = (level: string) => {
    switch (level) {
      case 'CRITICAL':
        return 'error';
      case 'WARNING':
        return 'warning';
      default:
        return 'success';
    }
  };

  const calculateSuccessRate = () => {
    if (!statistics || statistics.totalUnlockAttempts === 0) return 0;
    return Math.round((statistics.successfulUnlocks / statistics.totalUnlockAttempts) * 100);
  };

  const exportAnalytics = async () => {
    try {
      // TODO: Implement actual export functionality
      console.log('Exporting analytics data...');
    } catch (err) {
      setError('Failed to export analytics data');
    }
  };

  if (!statistics) {
    return (
      <Box sx={{ p: 3 }}>
        <LinearProgress />
      </Box>
    );
  }

  return (
    <Box sx={{ p: 3 }}>
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 3 }}>
        <Typography variant="h3" sx={{ fontWeight: 600 }}>
          Analytics Dashboard
        </Typography>
        <Stack direction="row" spacing={2}>
          <FormControl size="small" sx={{ minWidth: 120 }}>
            <InputLabel>Time Range</InputLabel>
            <Select
              value={timeRange}
              label="Time Range"
              onChange={(e) => setTimeRange(e.target.value)}
            >
              <MenuItem value="1d">Last 24 hours</MenuItem>
              <MenuItem value="7d">Last 7 days</MenuItem>
              <MenuItem value="30d">Last 30 days</MenuItem>
              <MenuItem value="90d">Last 90 days</MenuItem>
            </Select>
          </FormControl>
          <Button
            variant="outlined"
            startIcon={<DownloadIcon />}
            onClick={exportAnalytics}
          >
            Export Data
          </Button>
        </Stack>
      </Box>

      {loading && <LinearProgress sx={{ mb: 2 }} />}
      
      {error && (
        <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
          {error}
        </Alert>
      )}

      <Grid container spacing={3}>
        {/* Key Metrics */}
        <Grid item xs={12} md={3}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 1 }}>
                <FolderIcon color="primary" />
                <Typography variant="h6">Total Folders</Typography>
              </Stack>
              <Typography variant="h3" sx={{ fontWeight: 600, color: 'primary.main' }}>
                {statistics.totalFolders}
              </Typography>
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                Across {statistics.totalProfiles} profiles
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        <Grid item xs={12} md={3}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 1 }}>
                <CheckCircleIcon color="success" />
                <Typography variant="h6">Success Rate</Typography>
              </Stack>
              <Typography variant="h3" sx={{ fontWeight: 600, color: 'success.main' }}>
                {calculateSuccessRate()}%
              </Typography>
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                {statistics.successfulUnlocks}/{statistics.totalUnlockAttempts} unlocks
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        <Grid item xs={12} md={3}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 1 }}>
                <TrendingUpIcon color="info" />
                <Typography variant="h6">Detections</Typography>
              </Stack>
              <Typography variant="h3" sx={{ fontWeight: 600, color: 'info.main' }}>
                {statistics.keyboardSequenceDetections}
              </Typography>
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                Ctrl+Alt+V sequences
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        <Grid item xs={12} md={3}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 1 }}>
                <SecurityIcon color="error" />
                <Typography variant="h6">Security Events</Typography>
              </Stack>
              <Typography variant="h3" sx={{ fontWeight: 600, color: 'error.main' }}>
                {statistics.securityViolations}
              </Typography>
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                Violations detected
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        {/* Activity Chart */}
        <Grid item xs={12} md={8}>
          <Card>
            <CardContent>
              <Typography variant="h6" sx={{ mb: 3 }}>
                Daily Activity
              </Typography>
              <Box sx={{ height: 300, display: 'flex', alignItems: 'end', gap: 1 }}>
                {activityData.map((day, index) => (
                  <Box key={day.date} sx={{ flex: 1, textAlign: 'center' }}>
                    <Stack spacing={0.5} sx={{ height: 250, justifyContent: 'end' }}>
                      <Box
                        sx={{
                          height: `${(day.unlocks / 20) * 100}%`,
                          bgcolor: 'success.main',
                          borderRadius: 1,
                          minHeight: 4,
                        }}
                      />
                      <Box
                        sx={{
                          height: `${(day.detections / 10) * 50}%`,
                          bgcolor: 'info.main',
                          borderRadius: 1,
                          minHeight: 2,
                        }}
                      />
                      <Box
                        sx={{
                          height: `${(day.violations / 5) * 30}%`,
                          bgcolor: 'error.main',
                          borderRadius: 1,
                          minHeight: day.violations > 0 ? 2 : 0,
                        }}
                      />
                    </Stack>
                    <Typography variant="caption" sx={{ mt: 1, display: 'block' }}>
                      {new Date(day.date).toLocaleDateString('en-US', { month: 'short', day: 'numeric' })}
                    </Typography>
                  </Box>
                ))}
              </Box>
              <Stack direction="row" spacing={3} sx={{ mt: 2, justifyContent: 'center' }}>
                <Stack direction="row" alignItems="center" spacing={1}>
                  <Box sx={{ width: 12, height: 12, bgcolor: 'success.main', borderRadius: 1 }} />
                  <Typography variant="body2">Unlocks</Typography>
                </Stack>
                <Stack direction="row" alignItems="center" spacing={1}>
                  <Box sx={{ width: 12, height: 12, bgcolor: 'info.main', borderRadius: 1 }} />
                  <Typography variant="body2">Detections</Typography>
                </Stack>
                <Stack direction="row" alignItems="center" spacing={1}>
                  <Box sx={{ width: 12, height: 12, bgcolor: 'error.main', borderRadius: 1 }} />
                  <Typography variant="body2">Violations</Typography>
                </Stack>
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* System Information */}
        <Grid item xs={12} md={4}>
          <Card>
            <CardContent>
              <Typography variant="h6" sx={{ mb: 2 }}>
                System Information
              </Typography>
              <Stack spacing={2}>
                <Box>
                  <Typography variant="body2" sx={{ opacity: 0.7 }}>
                    Total Uptime
                  </Typography>
                  <Typography variant="h6">
                    {statistics.totalUptime}
                  </Typography>
                </Box>
                <Divider />
                <Box>
                  <Typography variant="body2" sx={{ opacity: 0.7 }}>
                    First Use
                  </Typography>
                  <Typography variant="body1">
                    {new Date(statistics.firstUse).toLocaleDateString()}
                  </Typography>
                </Box>
                <Divider />
                <Box>
                  <Typography variant="body2" sx={{ opacity: 0.7 }}>
                    Last Activity
                  </Typography>
                  <Typography variant="body1">
                    {statistics.lastActivity}
                  </Typography>
                </Box>
                <Divider />
                <Box>
                  <Typography variant="body2" sx={{ opacity: 0.7 }}>
                    Memory Usage
                  </Typography>
                  <Typography variant="body1">
                    5.6 MB
                  </Typography>
                  <LinearProgress 
                    variant="determinate" 
                    value={56} 
                    sx={{ mt: 1 }}
                    color="success"
                  />
                  <Typography variant="caption" sx={{ opacity: 0.7 }}>
                    56% of 10MB target
                  </Typography>
                </Box>
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* Recent Security Events */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Typography variant="h6" sx={{ mb: 2 }}>
                Recent Security Events
              </Typography>
              <TableContainer>
                <Table>
                  <TableHead>
                    <TableRow>
                      <TableCell>Event</TableCell>
                      <TableCell>Level</TableCell>
                      <TableCell>Description</TableCell>
                      <TableCell>Profile</TableCell>
                      <TableCell>Timestamp</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {securityEvents.map((event) => (
                      <TableRow key={event.id}>
                        <TableCell>
                          <Stack direction="row" alignItems="center" spacing={1}>
                            {getEventIcon(event.type)}
                            <Typography variant="body2">
                              {event.type.replace(/_/g, ' ')}
                            </Typography>
                          </Stack>
                        </TableCell>
                        <TableCell>
                          <Chip
                            label={event.level}
                            color={getEventColor(event.level) as any}
                            size="small"
                          />
                        </TableCell>
                        <TableCell>
                          <Typography variant="body2">
                            {event.description}
                          </Typography>
                        </TableCell>
                        <TableCell>
                          <Typography variant="body2">
                            {event.profileId || '-'}
                          </Typography>
                        </TableCell>
                        <TableCell>
                          <Typography variant="body2">
                            {event.timestamp}
                          </Typography>
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default Analytics;