#include "network.h"
#include "utils.h"
#include "logger.h"
#include <string.h>

void network_stats_init(NetworkStats *stats) {
    if (!stats) return;
    stats->latency_ms = 0.0;
    stats->jitter_ms = 0.0;
    stats->packet_loss_pct = 0.0;
    stats->bandwidth_util_pct = 0.0;
    stats->throughput_mbps = 0.0;
    stats->error_frames = 0;
    stats->is_simulated = true;
}

void network_simulate_telemetry(NetworkStats *stats, ScenarioType scenario) {
    if (!stats) return;
    
    stats->is_simulated = true;
    
    switch (scenario) {
        case SCENARIO_NORMAL:
            /* Healthy enterprise / backbone network conditions */
            stats->latency_ms = utils_random_double(2.0, 18.0);
            stats->jitter_ms = utils_random_double(0.2, 2.5);
            stats->packet_loss_pct = utils_random_double(0.0, 0.05);
            stats->bandwidth_util_pct = utils_random_double(20.0, 65.0);
            stats->throughput_mbps = utils_random_double(450.0, 950.0);
            stats->error_frames = 0;
            break;
            
        case SCENARIO_WARNING:
            /* Moderate congestion / link degradation */
            stats->latency_ms = utils_random_double(55.0, 95.0);
            stats->jitter_ms = utils_random_double(12.0, 28.0);
            stats->packet_loss_pct = utils_random_double(1.5, 4.5);
            stats->bandwidth_util_pct = utils_random_double(82.0, 92.0);
            stats->throughput_mbps = utils_random_double(850.0, 980.0);
            stats->error_frames = (uint64_t)utils_random_int(5, 50);
            break;
            
        case SCENARIO_CRITICAL:
            /* Severe network impairment / heavy packet loss / buffer exhaustion */
            stats->latency_ms = utils_random_double(160.0, 480.0);
            stats->jitter_ms = utils_random_double(45.0, 120.0);
            stats->packet_loss_pct = utils_random_double(8.0, 35.0);
            stats->bandwidth_util_pct = utils_random_double(96.0, 100.0);
            stats->throughput_mbps = utils_random_double(120.0, 350.0);
            stats->error_frames = (uint64_t)utils_random_int(150, 4500);
            break;
            
        case SCENARIO_FAULT_SPIKE:
            /* Transient burst / routing flap simulation */
            stats->latency_ms = utils_random_double(250.0, 600.0);
            stats->jitter_ms = utils_random_double(80.0, 200.0);
            stats->packet_loss_pct = utils_random_double(15.0, 50.0);
            stats->bandwidth_util_pct = utils_random_double(90.0, 99.0);
            stats->throughput_mbps = utils_random_double(50.0, 200.0);
            stats->error_frames = (uint64_t)utils_random_int(500, 10000);
            break;
    }
}

int network_collect_telemetry(NetworkStats *stats, const char *interface_or_host, bool simulate) {
    if (!stats) return -1;
    
    if (simulate) {
        /*
         * Synthetic Telemetry Generation:
         * Generates baseline normal telemetry with occasional random fluctuations.
         */
        int roll = utils_random_int(1, 100);
        if (roll <= 80) {
            network_simulate_telemetry(stats, SCENARIO_NORMAL);
        } else if (roll <= 95) {
            network_simulate_telemetry(stats, SCENARIO_WARNING);
        } else {
            network_simulate_telemetry(stats, SCENARIO_CRITICAL);
        }
        return 0;
    }
    
    /*
     * Extensible Hardware/Socket Hook:
     * When hardware/live interface telemetry is integrated in future releases,
     * this section will query socket ICMP echo (ping), Linux sysfs (/sys/class/net),
     * or AF_PACKET/ethtool stats.
     */
    LOG_WARN("Physical network telemetry driver not implemented yet for host '%s'. Falling back to simulation.",
             interface_or_host ? interface_or_host : "unknown");
    network_simulate_telemetry(stats, SCENARIO_NORMAL);
    stats->is_simulated = true;
    return 0;
}

const char *scenario_type_to_string(ScenarioType scenario) {
    switch (scenario) {
        case SCENARIO_NORMAL:      return "NORMAL";
        case SCENARIO_WARNING:     return "WARNING";
        case SCENARIO_CRITICAL:    return "CRITICAL";
        case SCENARIO_FAULT_SPIKE: return "FAULT_SPIKE";
        default:                   return "UNKNOWN";
    }
}
