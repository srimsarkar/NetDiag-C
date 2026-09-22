#ifndef NETDIAG_MONITORING_H
#define NETDIAG_MONITORING_H

#include <stdbool.h>
#include "device.h"
#include "diagnostics.h"

/**
 * Monitoring Engine Configuration options.
 */
typedef struct {
    char config_file[256];
    char device_csv[256];
    char log_file[256];
    int interval_seconds;
    bool simulate_all;
    bool verbose;
} MonitoringConfig;

/**
 * Core Monitoring Engine orchestrator.
 */
typedef struct {
    MonitoringConfig config;
    DeviceRegistry registry;
    ThresholdConfig thresholds;
    unsigned long cycle_count;
    bool is_running;
} MonitoringEngine;

/**
 * Initializes the monitoring engine with config files and device inventory.
 *
 * @param engine Pointer to MonitoringEngine structure.
 * @param config_file Path to thresholds.conf.
 * @param device_csv Path to sample_devices.csv.
 * @return 0 on success, negative error code on failure.
 */
int monitoring_init(MonitoringEngine *engine, const char *config_file, const char *device_csv);

/**
 * Executes a single monitoring cycle across all registered devices:
 * - Collects telemetry (simulated/physical)
 * - Evaluates diagnostic rules and thresholds
 * - Updates device statuses and logs anomalies
 *
 * @param engine Active MonitoringEngine.
 * @return 0 on success.
 */
int monitoring_run_cycle(MonitoringEngine *engine);

/**
 * Executes a comprehensive scenario matrix demonstrating NORMAL, WARNING, 
 * and CRITICAL diagnostic states across different device classes.
 *
 * @param engine Active MonitoringEngine.
 * @return 0 on success.
 */
int monitoring_run_scenario_matrix(MonitoringEngine *engine);

/**
 * Prints a formatted CLI dashboard summary table of all monitored devices.
 *
 * @param engine Active MonitoringEngine.
 */
void monitoring_print_summary(const MonitoringEngine *engine);

/**
 * Prints a detailed diagnostic report for a specific device.
 *
 * @param dev Pointer to Device.
 */
void monitoring_print_device_detail(const Device *dev);

/**
 * Cleans up monitoring engine resources.
 */
void monitoring_cleanup(MonitoringEngine *engine);

#endif /* NETDIAG_MONITORING_H */
