#include "diagnostics.h"
#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void set_default_thresholds(ThresholdConfig *cfg) {
    if (!cfg) return;
    
    /* Default Network Thresholds */
    cfg->latency_warning_ms       = 50.0;
    cfg->latency_critical_ms      = 150.0;
    cfg->packet_loss_warning_pct  = 1.0;
    cfg->packet_loss_critical_pct = 5.0;
    cfg->jitter_warning_ms        = 10.0;
    cfg->jitter_critical_ms       = 30.0;
    cfg->bandwidth_warning_pct    = 80.0;
    cfg->bandwidth_critical_pct   = 95.0;

    /* Default Optical Transceiver Thresholds (SFF-8472 / SFF-8636 standard ranges) */
    cfg->rx_power_low_warning_dbm  = -17.0;
    cfg->rx_power_low_critical_dbm = -22.0;
    cfg->rx_power_high_warning_dbm = 0.0;
    cfg->rx_power_high_critical_dbm= 2.0;

    cfg->tx_power_low_warning_dbm  = -7.0;
    cfg->tx_power_low_critical_dbm = -10.0;
    cfg->tx_power_high_warning_dbm = 1.0;
    cfg->tx_power_high_critical_dbm= 3.0;

    cfg->laser_bias_warning_ma     = 55.0;
    cfg->laser_bias_critical_ma    = 80.0;

    cfg->temp_warning_c            = 65.0;
    cfg->temp_critical_c           = 75.0;

    cfg->voltage_low_warning_v     = 3.15;
    cfg->voltage_low_critical_v    = 3.00;
    cfg->voltage_high_warning_v    = 3.45;
    cfg->voltage_high_critical_v   = 3.60;
}

int diagnostics_load_thresholds(const char *config_path, ThresholdConfig *cfg) {
    if (!cfg) return -1;
    
    set_default_thresholds(cfg);
    
    if (!config_path) {
        LOG_INFO("No configuration path provided; using built-in default thresholds.");
        return 0;
    }
    
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        LOG_WARN("Could not open threshold config file '%s'. Using built-in defaults.", config_path);
        return -1;
    }
    
    char line[256];
    int lines_parsed = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = utils_trim(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#' || trimmed[0] == '[') {
            continue; /* Skip empty lines, comments, and INI sections */
        }
        
        char *eq = strchr(trimmed, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = utils_trim(trimmed);
        char *val_str = utils_trim(eq + 1);
        double val = utils_parse_double(val_str, 0.0);
        
        if (strcmp(key, "latency_warning_ms") == 0)            cfg->latency_warning_ms = val;
        else if (strcmp(key, "latency_critical_ms") == 0)       cfg->latency_critical_ms = val;
        else if (strcmp(key, "packet_loss_warning_pct") == 0)   cfg->packet_loss_warning_pct = val;
        else if (strcmp(key, "packet_loss_critical_pct") == 0)  cfg->packet_loss_critical_pct = val;
        else if (strcmp(key, "jitter_warning_ms") == 0)         cfg->jitter_warning_ms = val;
        else if (strcmp(key, "jitter_critical_ms") == 0)        cfg->jitter_critical_ms = val;
        else if (strcmp(key, "bandwidth_warning_pct") == 0)     cfg->bandwidth_warning_pct = val;
        else if (strcmp(key, "bandwidth_critical_pct") == 0)    cfg->bandwidth_critical_pct = val;
        else if (strcmp(key, "rx_power_low_warning_dbm") == 0)  cfg->rx_power_low_warning_dbm = val;
        else if (strcmp(key, "rx_power_low_critical_dbm") == 0) cfg->rx_power_low_critical_dbm = val;
        else if (strcmp(key, "rx_power_high_warning_dbm") == 0) cfg->rx_power_high_warning_dbm = val;
        else if (strcmp(key, "rx_power_high_critical_dbm") == 0)cfg->rx_power_high_critical_dbm = val;
        else if (strcmp(key, "tx_power_low_warning_dbm") == 0)  cfg->tx_power_low_warning_dbm = val;
        else if (strcmp(key, "tx_power_low_critical_dbm") == 0) cfg->tx_power_low_critical_dbm = val;
        else if (strcmp(key, "tx_power_high_warning_dbm") == 0) cfg->tx_power_high_warning_dbm = val;
        else if (strcmp(key, "tx_power_high_critical_dbm") == 0)cfg->tx_power_high_critical_dbm = val;
        else if (strcmp(key, "laser_bias_warning_ma") == 0)     cfg->laser_bias_warning_ma = val;
        else if (strcmp(key, "laser_bias_critical_ma") == 0)    cfg->laser_bias_critical_ma = val;
        else if (strcmp(key, "temp_warning_c") == 0)            cfg->temp_warning_c = val;
        else if (strcmp(key, "temp_critical_c") == 0)           cfg->temp_critical_c = val;
        else if (strcmp(key, "voltage_low_warning_v") == 0)     cfg->voltage_low_warning_v = val;
        else if (strcmp(key, "voltage_low_critical_v") == 0)    cfg->voltage_low_critical_v = val;
        else if (strcmp(key, "voltage_high_warning_v") == 0)    cfg->voltage_high_warning_v = val;
        else if (strcmp(key, "voltage_high_critical_v") == 0)   cfg->voltage_high_critical_v = val;
        
        lines_parsed++;
    }
    
    fclose(fp);
    LOG_INFO("Loaded threshold configuration from '%s' (%d parameters)", config_path, lines_parsed);
    return 0;
}

void diagnostics_add_issue(DiagnosticResult *result, DiagnosticSeverity severity, 
                          const char *code, const char *fmt, ...) {
    if (!result || result->issue_count >= MAX_DIAGNOSTIC_ISSUES) return;
    
    DiagnosticIssue *issue = &result->issues[result->issue_count++];
    issue->severity = severity;
    snprintf(issue->code, sizeof(issue->code), "%s", code ? code : "ISSUE_GENERIC");
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(issue->description, sizeof(issue->description), fmt, args);
    va_end(args);
    
    /* Escalate overall status if issue is more severe */
    if (severity == SEV_CRITICAL) {
        result->overall_status = DEVICE_STATUS_CRITICAL;
    } else if (severity == SEV_WARNING && result->overall_status != DEVICE_STATUS_CRITICAL) {
        result->overall_status = DEVICE_STATUS_WARNING;
    }
}

void diagnostics_add_recommendation(DiagnosticResult *result, const char *fmt, ...) {
    if (!result || result->recommendation_count >= MAX_DIAGNOSTIC_RECOMMENDATIONS) return;
    
    char *rec = result->recommendations[result->recommendation_count++];
    va_list args;
    va_start(args, fmt);
    vsnprintf(rec, DIAG_MSG_LEN, fmt, args);
    va_end(args);
}

DiagnosticResult diagnostics_evaluate(const NetworkStats *net_stats, 
                                     const Transceiver *xcvr, 
                                     const ThresholdConfig *cfg) {
    DiagnosticResult res;
    memset(&res, 0, sizeof(DiagnosticResult));
    res.overall_status = DEVICE_STATUS_OK;
    res.evaluated_at = time(NULL);
    
    ThresholdConfig default_cfg;
    if (!cfg) {
        set_default_thresholds(&default_cfg);
        cfg = &default_cfg;
    }
    
    /* 1. Evaluate Network Layer Telemetry */
    if (net_stats) {
        /* Latency check */
        if (net_stats->latency_ms >= cfg->latency_critical_ms) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "NET_LATENCY_CRIT",
                                 "Extreme round-trip latency (%.1f ms >= critical threshold %.1f ms)",
                                 net_stats->latency_ms, cfg->latency_critical_ms);
            diagnostics_add_recommendation(&res, "Inspect intermediate routing hops and upstream peering congestion.");
        } else if (net_stats->latency_ms >= cfg->latency_warning_ms) {
            diagnostics_add_issue(&res, SEV_WARNING, "NET_LATENCY_WARN",
                                 "Elevated latency (%.1f ms >= warning threshold %.1f ms)",
                                 net_stats->latency_ms, cfg->latency_warning_ms);
            diagnostics_add_recommendation(&res, "Monitor traffic queue depths and QoS priority mappings.");
        }
        
        /* Packet loss check */
        if (net_stats->packet_loss_pct >= cfg->packet_loss_critical_pct) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "NET_PKTLOSS_CRIT",
                                 "Severe packet drop rate (%.2f%% >= critical threshold %.2f%%)",
                                 net_stats->packet_loss_pct, cfg->packet_loss_critical_pct);
            diagnostics_add_recommendation(&res, "Check for physical link degradation, interface framing errors, or link flapping.");
        } else if (net_stats->packet_loss_pct >= cfg->packet_loss_warning_pct) {
            diagnostics_add_issue(&res, SEV_WARNING, "NET_PKTLOSS_WARN",
                                 "Elevated packet loss (%.2f%% >= warning threshold %.2f%%)",
                                 net_stats->packet_loss_pct, cfg->packet_loss_warning_pct);
            diagnostics_add_recommendation(&res, "Investigate interface tail drops and rate limiting polices.");
        }
        
        /* Jitter check */
        if (net_stats->jitter_ms >= cfg->jitter_critical_ms) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "NET_JITTER_CRIT",
                                 "Excessive delay variance / jitter (%.1f ms >= %.1f ms)",
                                 net_stats->jitter_ms, cfg->jitter_critical_ms);
            diagnostics_add_recommendation(&res, "Check for micro-bursting traffic patterns and buffer oversubscription.");
        } else if (net_stats->jitter_ms >= cfg->jitter_warning_ms) {
            diagnostics_add_issue(&res, SEV_WARNING, "NET_JITTER_WARN",
                                 "Noticeable jitter detected (%.1f ms >= %.1f ms)",
                                 net_stats->jitter_ms, cfg->jitter_warning_ms);
        }
        
        /* Bandwidth utilization check */
        if (net_stats->bandwidth_util_pct >= cfg->bandwidth_critical_pct) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "NET_BW_SATURATED",
                                 "Link bandwidth near saturation (%.1f%% >= %.1f%%)",
                                 net_stats->bandwidth_util_pct, cfg->bandwidth_critical_pct);
            diagnostics_add_recommendation(&res, "Trigger link aggregation / ECMP re-balancing or throttle non-critical flows.");
        } else if (net_stats->bandwidth_util_pct >= cfg->bandwidth_warning_pct) {
            diagnostics_add_issue(&res, SEV_WARNING, "NET_BW_HIGH",
                                 "High link utilization (%.1f%% >= %.1f%%)",
                                 net_stats->bandwidth_util_pct, cfg->bandwidth_warning_pct);
        }
        
        /* Error frames */
        if (net_stats->error_frames > 50) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "NET_CRC_ERRORS",
                                 "High number of FCS/CRC error frames recorded (%lu frames)",
                                 (unsigned long)net_stats->error_frames);
            diagnostics_add_recommendation(&res, "Inspect physical cabling, patch panel seating, and transceiver optics.");
        } else if (net_stats->error_frames > 0) {
            diagnostics_add_issue(&res, SEV_WARNING, "NET_FRAME_ERRORS",
                                 "Sporadic framing errors detected (%lu frames)",
                                 (unsigned long)net_stats->error_frames);
        }
    }
    
    /* 2. Evaluate Optical Transceiver Telemetry (DDM/DOM) */
    if (xcvr) {
        /* Optical Received Power (RX Power) */
        if (xcvr->rx_power_dbm <= cfg->rx_power_low_critical_dbm) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "OPT_RX_PWR_CRIT_LOW",
                                 "Optical RX power severely degraded (%.2f dBm <= critical low %.2f dBm)",
                                 xcvr->rx_power_dbm, cfg->rx_power_low_critical_dbm);
            diagnostics_add_recommendation(&res, "Check for severe fiber macro-bends, dirty LC connectors, or upstream laser failure.");
        } else if (xcvr->rx_power_dbm <= cfg->rx_power_low_warning_dbm) {
            diagnostics_add_issue(&res, SEV_WARNING, "OPT_RX_PWR_WARN_LOW",
                                 "Optical RX power marginal (%.2f dBm <= warning low %.2f dBm)",
                                 xcvr->rx_power_dbm, cfg->rx_power_low_warning_dbm);
            diagnostics_add_recommendation(&res, "Clean optical fiber end-faces with one-click cleaner and verify patch cables.");
        } else if (xcvr->rx_power_dbm >= cfg->rx_power_high_critical_dbm) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "OPT_RX_PWR_OVERLOAD",
                                 "Optical RX power receiver overload (%.2f dBm >= %.2f dBm)",
                                 xcvr->rx_power_dbm, cfg->rx_power_high_critical_dbm);
            diagnostics_add_recommendation(&res, "Install an in-line optical attenuator to prevent photodiode damage.");
        }
        
        /* Optical Transmit Power (TX Power) */
        if (xcvr->tx_power_dbm <= cfg->tx_power_low_critical_dbm) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "OPT_TX_PWR_CRIT_LOW",
                                 "Optical TX power dying (%.2f dBm <= critical low %.2f dBm)",
                                 xcvr->tx_power_dbm, cfg->tx_power_low_critical_dbm);
            diagnostics_add_recommendation(&res, "Transmitter laser diode failure imminent. Schedule transceiver replacement.");
        } else if (xcvr->tx_power_dbm <= cfg->tx_power_low_warning_dbm) {
            diagnostics_add_issue(&res, SEV_WARNING, "OPT_TX_PWR_WARN_LOW",
                                 "Optical TX power sub-optimal (%.2f dBm <= warning low %.2f dBm)",
                                 xcvr->tx_power_dbm, cfg->tx_power_low_warning_dbm);
        }
        
        /* Laser Bias Current */
        if (xcvr->laser_bias_current_ma >= cfg->laser_bias_critical_ma) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "OPT_LASER_BIAS_CRIT",
                                 "Laser bias current saturation (%.1f mA >= %.1f mA)",
                                 xcvr->laser_bias_current_ma, cfg->laser_bias_critical_ma);
            diagnostics_add_recommendation(&res, "Laser diode is aging out of spec; replace optical module S/N %s.", xcvr->serial_number);
        } else if (xcvr->laser_bias_current_ma >= cfg->laser_bias_warning_ma) {
            diagnostics_add_issue(&res, SEV_WARNING, "OPT_LASER_BIAS_WARN",
                                 "Elevated laser bias current (%.1f mA >= %.1f mA)",
                                 xcvr->laser_bias_current_ma, cfg->laser_bias_warning_ma);
        }
        
        /* Transceiver Temperature */
        if (xcvr->temperature_c >= cfg->temp_critical_c) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "OPT_TEMP_CRITICAL",
                                 "Transceiver thermal emergency (%.1f C >= %.1f C)",
                                 xcvr->temperature_c, cfg->temp_critical_c);
            diagnostics_add_recommendation(&res, "Check chassis fan tray operation, air filters, and rack ambient cooling.");
        } else if (xcvr->temperature_c >= cfg->temp_warning_c) {
            diagnostics_add_issue(&res, SEV_WARNING, "OPT_TEMP_WARNING",
                                 "Transceiver operating temperature high (%.1f C >= %.1f C)",
                                 xcvr->temperature_c, cfg->temp_warning_c);
            diagnostics_add_recommendation(&res, "Verify airflow around optical cages.");
        }
        
        /* Voltage rail */
        if (xcvr->voltage_v <= cfg->voltage_low_critical_v || xcvr->voltage_v >= cfg->voltage_high_critical_v) {
            diagnostics_add_issue(&res, SEV_CRITICAL, "OPT_VOLTAGE_FAULT",
                                 "Transceiver supply voltage out of bounds (%.2f V)", xcvr->voltage_v);
            diagnostics_add_recommendation(&res, "Check switch line-card power rail regulators.");
        } else if (xcvr->voltage_v <= cfg->voltage_low_warning_v || xcvr->voltage_v >= cfg->voltage_high_warning_v) {
            diagnostics_add_issue(&res, SEV_WARNING, "OPT_VOLTAGE_WARN",
                                 "Transceiver supply voltage fluctuating (%.2f V)", xcvr->voltage_v);
        }
    }
    
    /* If no issues detected, provide confirmation recommendation */
    if (res.issue_count == 0) {
        diagnostics_add_recommendation(&res, "All telemetry parameters are within optimal operating margins.");
    }
    
    return res;
}

const char *device_status_to_string(DeviceStatus status) {
    switch (status) {
        case DEVICE_STATUS_OK:       return "HEALTHY (OK)";
        case DEVICE_STATUS_WARNING:  return "WARNING";
        case DEVICE_STATUS_CRITICAL: return "CRITICAL";
        case DEVICE_STATUS_UNKNOWN:  return "UNKNOWN";
        default:                     return "UNDEFINED";
    }
}

const char *diagnostic_severity_to_string(DiagnosticSeverity severity) {
    switch (severity) {
        case SEV_INFO:     return "INFO";
        case SEV_WARNING:  return "WARNING";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "UNKNOWN";
    }
}
