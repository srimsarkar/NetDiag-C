#include "monitoring.h"
#include "logger.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int monitoring_init(MonitoringEngine *engine, const char *config_file, const char *device_csv) {
    if (!engine) return -1;
    
    memset(engine, 0, sizeof(MonitoringEngine));
    
    if (config_file) {
        snprintf(engine->config.config_file, sizeof(engine->config.config_file), "%s", config_file);
    } else {
        snprintf(engine->config.config_file, sizeof(engine->config.config_file), "config/thresholds.conf");
    }
    
    if (device_csv) {
        snprintf(engine->config.device_csv, sizeof(engine->config.device_csv), "%s", device_csv);
    } else {
        snprintf(engine->config.device_csv, sizeof(engine->config.device_csv), "data/sample_devices.csv");
    }
    
    engine->config.interval_seconds = 5;
    engine->config.simulate_all = true;
    engine->is_running = true;
    engine->cycle_count = 0;
    
    /* Load Threshold Configuration */
    diagnostics_load_thresholds(engine->config.config_file, &engine->thresholds);
    
    /* Initialize Device Registry & Load Devices from CSV */
    device_registry_init(&engine->registry);
    int loaded = device_registry_load_csv(&engine->registry, engine->config.device_csv);
    if (loaded < 0) {
        LOG_WARN("Could not load devices from '%s'. Initializing default devices in-memory.", engine->config.device_csv);
        
        Device *d1 = device_registry_add(&engine->registry, "DEV-RTR-01", "Core Gateway Router", "10.0.0.1", DEVICE_TYPE_ROUTER, true);
        if (d1) transceiver_init(&d1->transceiver, "OPT-SN-88219", "QSFP28-100G-LR4", TRANSCEIVER_QSFP28, 1310.0);
        
        Device *d2 = device_registry_add(&engine->registry, "DEV-SW-02", "Spine Switch 01", "10.0.0.2", DEVICE_TYPE_SWITCH, true);
        if (d2) transceiver_init(&d2->transceiver, "OPT-SN-99102", "SFP-10G-SR", TRANSCEIVER_SFP_PLUS, 850.0);
        
        device_registry_add(&engine->registry, "DEV-SRV-03", "Application Cluster Node", "10.0.1.50", DEVICE_TYPE_SERVER, false);
    }
    
    LOG_INFO("Monitoring Engine initialized with %zu registered devices.", engine->registry.count);
    return 0;
}

int monitoring_run_cycle(MonitoringEngine *engine) {
    if (!engine) return -1;
    
    engine->cycle_count++;
    LOG_INFO("--- Starting Monitoring Cycle #%lu ---", engine->cycle_count);
    
    for (size_t i = 0; i < engine->registry.count; i++) {
        Device *dev = &engine->registry.devices[i];
        
        /* 1. Collect / Simulate Network Telemetry */
        network_collect_telemetry(&dev->network_stats, dev->ip_address, engine->config.simulate_all);
        
        /* 2. Collect / Simulate Optical Transceiver Telemetry if equipped */
        if (dev->has_transceiver) {
            transceiver_collect_telemetry(&dev->transceiver, dev->id, engine->config.simulate_all);
        }
        
        /* 3. Run Diagnostic Evaluation */
        dev->diagnostic_result = diagnostics_evaluate(
            &dev->network_stats,
            dev->has_transceiver ? &dev->transceiver : NULL,
            &engine->thresholds
        );
        
        /* 4. Update Device Status */
        dev->status = dev->diagnostic_result.overall_status;
        utils_get_timestamp(dev->last_updated, sizeof(dev->last_updated));
        
        /* Log any detected issues */
        if (dev->status == DEVICE_STATUS_CRITICAL) {
            LOG_CRITICAL("Device [%s] '%s' is in CRITICAL state! (%d issue(s) detected)",
                         dev->id, dev->name, dev->diagnostic_result.issue_count);
        } else if (dev->status == DEVICE_STATUS_WARNING) {
            LOG_WARN("Device [%s] '%s' is in WARNING state (%d issue(s) detected)",
                     dev->id, dev->name, dev->diagnostic_result.issue_count);
        }
    }
    
    return 0;
}

int monitoring_run_scenario_matrix(MonitoringEngine *engine) {
    if (!engine) return -1;
    
    LOG_INFO("================================================================================");
    LOG_INFO("   RUNNING NETDIAG-C DIAGNOSTIC SCENARIO MATRIX VERIFICATION");
    LOG_INFO("   (Simulating Normal, Warning, and Critical Operational Conditions)");
    LOG_INFO("================================================================================");
    
    ScenarioType scenarios[] = { SCENARIO_NORMAL, SCENARIO_WARNING, SCENARIO_CRITICAL, SCENARIO_FAULT_SPIKE };
    const char *scenario_names[] = {
        "SCENARIO 1: Nominal Network & Optical Performance (NORMAL)",
        "SCENARIO 2: Optical Fiber Degradation / Moderate Congestion (WARNING)",
        "SCENARIO 3: Link Saturation & Optical Laser Bias Failure (CRITICAL)",
        "SCENARIO 4: Routing Flap & Transceiver Optical Power Overload (FAULT_SPIKE)"
    };
    
    for (int s = 0; s < 4; s++) {
        printf("\n\n%s================================================================================%s\n", COLOR_CYAN, COLOR_RESET);
        printf("%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, scenario_names[s], COLOR_RESET);
        printf("%s================================================================================%s\n", COLOR_CYAN, COLOR_RESET);
        
        for (size_t i = 0; i < engine->registry.count; i++) {
            Device *dev = &engine->registry.devices[i];
            
            /* Apply synthetic scenario profile */
            network_simulate_telemetry(&dev->network_stats, scenarios[s]);
            if (dev->has_transceiver) {
                transceiver_simulate_telemetry(&dev->transceiver, scenarios[s]);
            }
            
            dev->diagnostic_result = diagnostics_evaluate(
                &dev->network_stats,
                dev->has_transceiver ? &dev->transceiver : NULL,
                &engine->thresholds
            );
            dev->status = dev->diagnostic_result.overall_status;
            utils_get_timestamp(dev->last_updated, sizeof(dev->last_updated));
        }
        
        monitoring_print_summary(engine);
        
        /* Print detail for the first device showing active diagnostics */
        if (engine->registry.count > 0) {
            printf("\n%s--- Diagnostic Deep-Dive for %s (%s) ---%s\n", COLOR_BOLD, engine->registry.devices[0].id, engine->registry.devices[0].name, COLOR_RESET);
            monitoring_print_device_detail(&engine->registry.devices[0]);
        }
    }
    
    return 0;
}

void monitoring_print_summary(const MonitoringEngine *engine) {
    if (!engine) return;
    
    printf("\n%s+-------------------------------------------------------------------------------------------------------------------------------+%s\n", COLOR_WHITE, COLOR_RESET);
    printf("| %s%-12s%s | %s%-20s%s | %s%-15s%s | %s%-8s%s | %s%-8s%s | %s%-11s%s | %s%-10s%s | %s%-14s%s |\n",
           COLOR_BOLD, "Device ID", COLOR_RESET,
           COLOR_BOLD, "Name", COLOR_RESET,
           COLOR_BOLD, "IP Address", COLOR_RESET,
           COLOR_BOLD, "RTT Lat", COLOR_RESET,
           COLOR_BOLD, "Pkt Loss", COLOR_RESET,
           COLOR_BOLD, "Opt RX Pwr", COLOR_RESET,
           COLOR_BOLD, "Opt Temp", COLOR_RESET,
           COLOR_BOLD, "Status", COLOR_RESET);
    printf("+-------------------------------------------------------------------------------------------------------------------------------+\n");
    
    for (size_t i = 0; i < engine->registry.count; i++) {
        const Device *dev = &engine->registry.devices[i];
        
        const char *status_color = COLOR_GREEN;
        const char *status_text = "OK";
        if (dev->status == DEVICE_STATUS_WARNING) {
            status_color = COLOR_YELLOW;
            status_text = "WARNING";
        } else if (dev->status == DEVICE_STATUS_CRITICAL) {
            status_color = COLOR_RED COLOR_BOLD;
            status_text = "CRITICAL";
        } else if (dev->status == DEVICE_STATUS_UNKNOWN) {
            status_color = COLOR_WHITE;
            status_text = "UNKNOWN";
        }
        
        char opt_rx_str[16] = "N/A";
        char opt_temp_str[16] = "N/A";
        if (dev->has_transceiver) {
            snprintf(opt_rx_str, sizeof(opt_rx_str), "%.1fdBm", dev->transceiver.rx_power_dbm);
            snprintf(opt_temp_str, sizeof(opt_temp_str), "%.1f°C", dev->transceiver.temperature_c);
        }
        
        printf("| %-12s | %-20.20s | %-15s | %6.1fms | %6.2f%% | %-11s | %-10s | %s%-14s%s |\n",
               dev->id,
               dev->name,
               dev->ip_address,
               dev->network_stats.latency_ms,
               dev->network_stats.packet_loss_pct,
               opt_rx_str,
               opt_temp_str,
               status_color, status_text, COLOR_RESET);
    }
    printf("+-------------------------------------------------------------------------------------------------------------------------------+\n");
}

void monitoring_print_device_detail(const Device *dev) {
    if (!dev) return;
    
    printf("\n%s[Device Metadata]%s\n", COLOR_BOLD COLOR_CYAN, COLOR_RESET);
    printf("  ID:           %s\n", dev->id);
    printf("  Name:         %s\n", dev->name);
    printf("  IP Address:   %s\n", dev->ip_address);
    printf("  Type:         %s\n", device_type_to_string(dev->type));
    printf("  Status:       %s\n", device_status_to_string(dev->status));
    printf("  Last Checked: %s\n", dev->last_updated);
    
    printf("\n%s[Network Performance Telemetry]%s (Simulated: %s)\n",
           COLOR_BOLD COLOR_BLUE, COLOR_RESET, dev->network_stats.is_simulated ? "YES" : "NO");
    printf("  Latency (RTT):        %6.2f ms\n", dev->network_stats.latency_ms);
    printf("  Jitter:               %6.2f ms\n", dev->network_stats.jitter_ms);
    printf("  Packet Loss:          %6.2f %%\n", dev->network_stats.packet_loss_pct);
    printf("  Bandwidth Utilization:%6.1f %%\n", dev->network_stats.bandwidth_util_pct);
    printf("  Throughput:           %6.1f Mbps\n", dev->network_stats.throughput_mbps);
    printf("  Error Frames (CRC):   %lu frames\n", (unsigned long)dev->network_stats.error_frames);
    
    if (dev->has_transceiver) {
        printf("\n%s[Optical Transceiver DDM Telemetry]%s (Simulated: %s)\n",
               COLOR_BOLD COLOR_MAGENTA, COLOR_RESET, dev->transceiver.is_simulated ? "YES (SFF-8472 Model)" : "NO (Hardware)");
        printf("  Serial Number:        %s\n", dev->transceiver.serial_number);
        printf("  Part Number:          %s\n", dev->transceiver.vendor_part_number);
        printf("  Form Factor:          %s\n", transceiver_form_factor_to_string(dev->transceiver.form_factor));
        printf("  Wavelength:           %.1f nm\n", dev->transceiver.wavelength_nm);
        printf("  RX Optical Power:     %6.2f dBm\n", dev->transceiver.rx_power_dbm);
        printf("  TX Optical Power:     %6.2f dBm\n", dev->transceiver.tx_power_dbm);
        printf("  Laser Bias Current:   %6.2f mA\n", dev->transceiver.laser_bias_current_ma);
        printf("  Internal Temperature: %6.1f °C\n", dev->transceiver.temperature_c);
        printf("  Supply Voltage:       %6.2f V\n", dev->transceiver.voltage_v);
    }
    
    printf("\n%s[Diagnostic Evaluation & Remediation Guidance]%s\n", COLOR_BOLD COLOR_YELLOW, COLOR_RESET);
    if (dev->diagnostic_result.issue_count == 0) {
        printf("  %s✓ No active anomalies detected. Device operating within nominal tolerances.%s\n",
               COLOR_GREEN, COLOR_RESET);
    } else {
        printf("  Detected Anomalies (%d):\n", dev->diagnostic_result.issue_count);
        for (int i = 0; i < dev->diagnostic_result.issue_count; i++) {
            const DiagnosticIssue *iss = &dev->diagnostic_result.issues[i];
            const char *col = (iss->severity == SEV_CRITICAL) ? COLOR_RED : COLOR_YELLOW;
            printf("    %s[%s] [%s] %s%s\n", col, diagnostic_severity_to_string(iss->severity), iss->code, iss->description, COLOR_RESET);
        }
    }
    
    if (dev->diagnostic_result.recommendation_count > 0) {
        printf("  Recommended Corrective Actions (%d):\n", dev->diagnostic_result.recommendation_count);
        for (int i = 0; i < dev->diagnostic_result.recommendation_count; i++) {
            printf("    %s→ %s%s\n", COLOR_CYAN, dev->diagnostic_result.recommendations[i], COLOR_RESET);
        }
    }
    printf("\n");
}

void monitoring_cleanup(MonitoringEngine *engine) {
    if (!engine) return;
    engine->is_running = false;
    LOG_INFO("Monitoring Engine stopped. Total completed cycles: %lu", engine->cycle_count);
}
