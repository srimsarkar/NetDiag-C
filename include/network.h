#ifndef NETDIAG_NETWORK_H
#define NETDIAG_NETWORK_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Diagnostic simulation scenario profiles.
 * Used for synthetic data generation and testing.
 */
typedef enum {
    SCENARIO_NORMAL = 0,
    SCENARIO_WARNING,
    SCENARIO_CRITICAL,
    SCENARIO_FAULT_SPIKE
} ScenarioType;

/**
 * Network telemetry statistics structure.
 * Encapsulates performance and reliability metrics of network links.
 */
typedef struct {
    double latency_ms;         /* Round-trip time latency in milliseconds */
    double jitter_ms;          /* Latency variance (jitter) in milliseconds */
    double packet_loss_pct;    /* Packet drop rate percentage (0.0 - 100.0) */
    double bandwidth_util_pct; /* Link utilization percentage (0.0 - 100.0) */
    double throughput_mbps;    /* Current link throughput in Megabits per second */
    uint64_t error_frames;     /* Cyclic redundancy check (CRC) / FCS error frames */
    bool is_simulated;         /* Flag indicating telemetry is synthetically generated */
} NetworkStats;

/**
 * Initializes a NetworkStats structure to default zero values.
 */
void network_stats_init(NetworkStats *stats);

/**
 * Simulates network telemetry metrics according to a given test scenario profile.
 * Generates bounded random values modeling realistic network conditions.
 *
 * @param stats Target structure to populate.
 * @param scenario Predefined test profile (NORMAL, WARNING, CRITICAL, SPIKE).
 */
void network_simulate_telemetry(NetworkStats *stats, ScenarioType scenario);

/**
 * Telemetry collector interface.
 * Extensible provider: currently queries simulation engine;
 * can be hooked to Linux socket/sysfs/ethtool driver in future versions.
 *
 * @param stats Target structure to populate.
 * @param interface_or_host Name of interface or host IP/FQDN.
 * @param simulate If true, synthetically generates data; if false, queries physical layer.
 * @return 0 on success, negative error code on failure.
 */
int network_collect_telemetry(NetworkStats *stats, const char *interface_or_host, bool simulate);

/**
 * Converts a ScenarioType enum to a string representation.
 */
const char *scenario_type_to_string(ScenarioType scenario);

#endif /* NETDIAG_NETWORK_H */
