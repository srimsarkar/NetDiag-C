#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "monitoring.h"
#include "logger.h"
#include "utils.h"

static void print_usage(const char *prog_name) {
    printf("NetDiag-C – C-Based Network Device Monitoring & Diagnostic System\n");
    printf("Usage: %s [OPTIONS]\n\n", prog_name);
    printf("Options:\n");
    printf("  --demo               Run the scenario matrix (Normal, Warning, Critical, Spike)\n");
    printf("  --single             Run a single diagnostic monitoring cycle and display reports\n");
    printf("  --cycles <N>         Run N periodic monitoring cycles\n");
    printf("  --interval <sec>     Interval between cycles in seconds (default: 3)\n");
    printf("  --config <path>      Path to threshold config file (default: config/thresholds.conf)\n");
    printf("  --devices <path>     Path to devices CSV file (default: data/sample_devices.csv)\n");
    printf("  --log <path>         Path to log file (default: logs/netdiag.log)\n");
    printf("  --detail <device_id> Print detailed report for a specific device\n");
    printf("  --verbose, -v        Enable verbose DEBUG logging\n");
    printf("  --help, -h           Display this help message and exit\n\n");
    printf("Note: Optical transceiver DDM metrics and network metrics are synthetically generated\n");
    printf("      in this version to simulate physical telemetry without requiring hardware.\n");
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));
    
    const char *config_file = "config/thresholds.conf";
    const char *devices_csv = "data/sample_devices.csv";
    const char *log_file = "logs/netdiag.log";
    const char *target_device_id = NULL;
    
    bool run_demo = false;
    bool single_cycle = false;
    int cycle_limit = 1;
    int interval_sec = 3;
    LogLevel log_level = LOG_LEVEL_INFO;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--demo") == 0) {
            run_demo = true;
        } else if (strcmp(argv[i], "--single") == 0 || strcmp(argv[i], "-s") == 0) {
            single_cycle = true;
            cycle_limit = 1;
        } else if (strcmp(argv[i], "--cycles") == 0 && i + 1 < argc) {
            cycle_limit = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            interval_sec = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        } else if (strcmp(argv[i], "--devices") == 0 && i + 1 < argc) {
            devices_csv = argv[++i];
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            log_file = argv[++i];
        } else if (strcmp(argv[i], "--detail") == 0 && i + 1 < argc) {
            target_device_id = argv[++i];
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            log_level = LOG_LEVEL_DEBUG;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    /* Initialize logging subsystem */
    logger_init(log_file, log_level);
    LOG_INFO("Starting NetDiag-C Diagnostic Engine...");
    
    /* Initialize monitoring engine */
    MonitoringEngine engine;
    if (monitoring_init(&engine, config_file, devices_csv) != 0) {
        LOG_CRITICAL("Failed to initialize monitoring engine.");
        logger_shutdown();
        return 1;
    }
    
    if (run_demo) {
        /* Run automated scenario matrix */
        monitoring_run_scenario_matrix(&engine);
    } else {
        /* Run monitoring cycles */
        for (int c = 0; c < cycle_limit; c++) {
            monitoring_run_cycle(&engine);
            monitoring_print_summary(&engine);
            
            if (target_device_id) {
                Device *dev = device_registry_find_by_id(&engine.registry, target_device_id);
                if (dev) {
                    monitoring_print_device_detail(dev);
                } else {
                    printf("Device ID '%s' not found.\n", target_device_id);
                }
            } else if (single_cycle || cycle_limit == 1) {
                /* Print detailed diagnostic view for all warning or critical devices */
                for (size_t i = 0; i < engine.registry.count; i++) {
                    Device *dev = &engine.registry.devices[i];
                    if (dev->status != DEVICE_STATUS_OK) {
                        monitoring_print_device_detail(dev);
                    }
                }
            }
            
            if (c + 1 < cycle_limit) {
                sleep((unsigned int)interval_sec);
            }
        }
    }
    
    monitoring_cleanup(&engine);
    logger_shutdown();
    return 0;
}
