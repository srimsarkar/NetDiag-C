#ifndef NETDIAG_DIAGNOSTICS_H
#define NETDIAG_DIAGNOSTICS_H

#include <stdbool.h>
#include <time.h>
#include "network.h"
#include "transceiver.h"

#define MAX_DIAGNOSTIC_ISSUES          16
#define MAX_DIAGNOSTIC_RECOMMENDATIONS 16
#define DIAG_MSG_LEN                   256

/* Forward declaration for Device struct to avoid circular dependency */
struct Device;

/**
 * Diagnostic Severity Level for specific anomalies.
 */
typedef enum {
    SEV_INFO = 0,
    SEV_WARNING,
    SEV_CRITICAL
} DiagnosticSeverity;

/**
 * Health status classification for monitored devices.
 */
typedef enum {
    DEVICE_STATUS_OK = 0,
    DEVICE_STATUS_WARNING,
    DEVICE_STATUS_CRITICAL,
    DEVICE_STATUS_UNKNOWN
} DeviceStatus;

/**
 * Configurable metric threshold boundaries for diagnostic evaluation.
 */
typedef struct {
    /* Network Thresholds */
    double latency_warning_ms;
    double latency_critical_ms;
    double packet_loss_warning_pct;
    double packet_loss_critical_pct;
    double jitter_warning_ms;
    double jitter_critical_ms;
    double bandwidth_warning_pct;
    double bandwidth_critical_pct;

    /* Optical Transceiver Thresholds (DDM/DOM) */
    double rx_power_low_warning_dbm;
    double rx_power_low_critical_dbm;
    double rx_power_high_warning_dbm;
    double rx_power_high_critical_dbm;

    double tx_power_low_warning_dbm;
    double tx_power_low_critical_dbm;
    double tx_power_high_warning_dbm;
    double tx_power_high_critical_dbm;

    double laser_bias_warning_ma;
    double laser_bias_critical_ma;

    double temp_warning_c;
    double temp_critical_c;

    double voltage_low_warning_v;
    double voltage_low_critical_v;
    double voltage_high_warning_v;
    double voltage_high_critical_v;
} ThresholdConfig;

/**
 * Individual detected diagnostic issue.
 */
typedef struct {
    DiagnosticSeverity severity;
    char code[32];
    char description[DIAG_MSG_LEN];
} DiagnosticIssue;

/**
 * Aggregate diagnostic evaluation result for a device.
 */
typedef struct {
    DeviceStatus overall_status;
    int issue_count;
    DiagnosticIssue issues[MAX_DIAGNOSTIC_ISSUES];
    int recommendation_count;
    char recommendations[MAX_DIAGNOSTIC_RECOMMENDATIONS][DIAG_MSG_LEN];
    time_t evaluated_at;
} DiagnosticResult;

/**
 * Loads diagnostic threshold limits from a configuration file.
 * Falls back to sensible default thresholds if file cannot be opened.
 *
 * @param config_path Path to thresholds.conf.
 * @param cfg Pointer to target ThresholdConfig structure.
 * @return 0 on success, negative code on fallback/error.
 */
int diagnostics_load_thresholds(const char *config_path, ThresholdConfig *cfg);

/**
 * Evaluates network and optical telemetry against thresholds to produce a DiagnosticResult.
 *
 * @param net_stats Network statistics.
 * @param xcvr Optical transceiver parameters (can be NULL if no transceiver).
 * @param cfg Active threshold limits.
 * @return Evaluated DiagnosticResult structure.
 */
DiagnosticResult diagnostics_evaluate(const NetworkStats *net_stats, 
                                     const Transceiver *xcvr, 
                                     const ThresholdConfig *cfg);

/**
 * Appends an issue to a diagnostic result.
 */
void diagnostics_add_issue(DiagnosticResult *result, DiagnosticSeverity severity, 
                          const char *code, const char *fmt, ...);

/**
 * Appends a recommended corrective action to a diagnostic result.
 */
void diagnostics_add_recommendation(DiagnosticResult *result, const char *fmt, ...);

/**
 * Converts a DeviceStatus enum to a formatted string.
 */
const char *device_status_to_string(DeviceStatus status);

/**
 * Converts a DiagnosticSeverity enum to a formatted string.
 */
const char *diagnostic_severity_to_string(DiagnosticSeverity severity);

#endif /* NETDIAG_DIAGNOSTICS_H */
