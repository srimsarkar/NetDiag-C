#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "diagnostics.h"
#include "network.h"
#include "transceiver.h"
#include "device.h"
#include "utils.h"
#include "logger.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(expr, msg) do { \
    g_tests_run++; \
    if (expr) { \
        g_tests_passed++; \
        printf("  %s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, msg); \
    } else { \
        printf("  %s[FAIL]%s %s (Line %d: %s)\n", COLOR_RED, COLOR_RESET, msg, __LINE__, #expr); \
    } \
} while(0)

static void test_normal_scenario_diagnostics(void) {
    printf("\n%s--- Running Test: Normal Scenario Diagnostics ---%s\n", COLOR_CYAN, COLOR_RESET);
    
    ThresholdConfig cfg;
    diagnostics_load_thresholds(NULL, &cfg);
    
    NetworkStats net;
    network_stats_init(&net);
    net.latency_ms = 8.5;
    net.jitter_ms = 0.8;
    net.packet_loss_pct = 0.0;
    net.bandwidth_util_pct = 35.0;
    net.error_frames = 0;
    
    Transceiver xcvr;
    transceiver_init(&xcvr, "SN-TEST-NORM", "SFP-10G-LR", TRANSCEIVER_SFP_PLUS, 1310.0);
    xcvr.rx_power_dbm = -10.0;
    xcvr.tx_power_dbm = -2.0;
    xcvr.laser_bias_current_ma = 32.0;
    xcvr.temperature_c = 40.0;
    xcvr.voltage_v = 3.30;
    
    DiagnosticResult res = diagnostics_evaluate(&net, &xcvr, &cfg);
    
    TEST_ASSERT(res.overall_status == DEVICE_STATUS_OK, "Overall status should be OK");
    TEST_ASSERT(res.issue_count == 0, "Issue count should be 0");
    TEST_ASSERT(res.recommendation_count > 0, "Recommendation should provide confirmation");
}

static void test_warning_scenario_diagnostics(void) {
    printf("\n%s--- Running Test: Warning Scenario Diagnostics ---%s\n", COLOR_CYAN, COLOR_RESET);
    
    ThresholdConfig cfg;
    diagnostics_load_thresholds(NULL, &cfg);
    
    /* Case A: Network Latency Warning */
    NetworkStats net;
    network_stats_init(&net);
    net.latency_ms = 65.0; /* Above warning (50.0), below critical (150.0) */
    
    DiagnosticResult res_net = diagnostics_evaluate(&net, NULL, &cfg);
    TEST_ASSERT(res_net.overall_status == DEVICE_STATUS_WARNING, "Elevated latency triggers WARNING status");
    TEST_ASSERT(res_net.issue_count == 1, "Detected 1 network warning issue");
    TEST_ASSERT(strcmp(res_net.issues[0].code, "NET_LATENCY_WARN") == 0, "Issue code matches NET_LATENCY_WARN");
    
    /* Case B: Optical RX Power Low Warning */
    Transceiver xcvr;
    transceiver_init(&xcvr, "SN-TEST-WARN", "SFP-10G-LR", TRANSCEIVER_SFP_PLUS, 1310.0);
    xcvr.rx_power_dbm = -18.5; /* Below warning low (-17.0), above critical low (-22.0) */
    
    network_stats_init(&net);
    net.latency_ms = 10.0;
    
    DiagnosticResult res_opt = diagnostics_evaluate(&net, &xcvr, &cfg);
    TEST_ASSERT(res_opt.overall_status == DEVICE_STATUS_WARNING, "Marginal optical RX power triggers WARNING status");
    TEST_ASSERT(res_opt.issue_count == 1, "Detected 1 optical warning issue");
    TEST_ASSERT(strcmp(res_opt.issues[0].code, "OPT_RX_PWR_WARN_LOW") == 0, "Issue code matches OPT_RX_PWR_WARN_LOW");
}

static void test_critical_scenario_diagnostics(void) {
    printf("\n%s--- Running Test: Critical Scenario Diagnostics ---%s\n", COLOR_CYAN, COLOR_RESET);
    
    ThresholdConfig cfg;
    diagnostics_load_thresholds(NULL, &cfg);
    
    /* High Packet Loss + Dying Optical Laser Bias */
    NetworkStats net;
    network_stats_init(&net);
    net.packet_loss_pct = 12.5; /* Above critical (5.0%) */
    net.latency_ms = 220.0;     /* Above critical (150.0ms) */
    
    Transceiver xcvr;
    transceiver_init(&xcvr, "SN-TEST-CRIT", "QSFP28-100G-LR4", TRANSCEIVER_QSFP28, 1310.0);
    xcvr.laser_bias_current_ma = 95.0; /* Above critical (80.0 mA) */
    xcvr.temperature_c = 82.0;         /* Above critical (75.0 C) */
    
    DiagnosticResult res = diagnostics_evaluate(&net, &xcvr, &cfg);
    TEST_ASSERT(res.overall_status == DEVICE_STATUS_CRITICAL, "Severe metrics trigger CRITICAL status");
    TEST_ASSERT(res.issue_count >= 4, "Multiple critical issues identified");
    TEST_ASSERT(res.recommendation_count >= 2, "Multiple actionable recommendations generated");
}

static void test_threshold_config_loader(void) {
    printf("\n%s--- Running Test: Threshold Config Loader ---%s\n", COLOR_CYAN, COLOR_RESET);
    
    ThresholdConfig cfg;
    int rc = diagnostics_load_thresholds("config/thresholds.conf", &cfg);
    TEST_ASSERT(rc == 0, "Threshold config loads successfully");
    TEST_ASSERT(cfg.latency_warning_ms > 0.0, "Latency warning threshold parsed");
    TEST_ASSERT(cfg.rx_power_low_warning_dbm < 0.0, "Optical RX power threshold parsed");
}

static void test_device_registry_and_parser(void) {
    printf("\n%s--- Running Test: Device Registry & Parsing ---%s\n", COLOR_CYAN, COLOR_RESET);
    
    DeviceRegistry reg;
    device_registry_init(&reg);
    TEST_ASSERT(reg.count == 0, "Registry starts empty");
    
    Device *d1 = device_registry_add(&reg, "TEST-R1", "Core Router", "192.168.1.1", DEVICE_TYPE_ROUTER, true);
    TEST_ASSERT(d1 != NULL, "Device added to registry");
    TEST_ASSERT(reg.count == 1, "Registry count is 1");
    
    Device *found = device_registry_find_by_id(&reg, "TEST-R1");
    TEST_ASSERT(found != NULL && strcmp(found->name, "Core Router") == 0, "Find by ID retrieves device");
    
    TEST_ASSERT(device_parse_type("ROUTER") == DEVICE_TYPE_ROUTER, "Parser maps ROUTER correctly");
    TEST_ASSERT(device_parse_type("SWITCH") == DEVICE_TYPE_SWITCH, "Parser maps SWITCH correctly");
    TEST_ASSERT(transceiver_parse_form_factor("QSFP28") == TRANSCEIVER_QSFP28, "Parser maps QSFP28 correctly");

    /* Test CSV line tokenizer with trailing empty columns */
    char test_csv_line[] = "DEV-TEST,Test Node,10.0.0.5,SERVER,false,,,,";
    char *tokens[10];
    int token_count = utils_split_csv_line(test_csv_line, tokens, 10);
    TEST_ASSERT(token_count == 9, "CSV parser extracts all 9 tokens including trailing empty fields");
    TEST_ASSERT(strcmp(tokens[0], "DEV-TEST") == 0, "First token is valid");
    TEST_ASSERT(strcmp(tokens[4], "false") == 0, "Fifth token is valid");
    TEST_ASSERT(strcmp(tokens[8], "") == 0, "Trailing empty token is an empty string");

    /* Test directory creation helper */
    int dir_rc = utils_ensure_parent_dir("logs/test_subdir/test.log");
    TEST_ASSERT(dir_rc == 0, "utils_ensure_parent_dir successfully creates parent directories");
}

static void test_simulation_bounds_and_disclaimer(void) {
    printf("\n%s--- Running Test: Simulation Bounds & Transparency ---%s\n", COLOR_CYAN, COLOR_RESET);
    
    NetworkStats net;
    network_stats_init(&net);
    network_simulate_telemetry(&net, SCENARIO_NORMAL);
    TEST_ASSERT(net.is_simulated == true, "Network stats explicitly marked as simulated");
    TEST_ASSERT(net.latency_ms >= 0.0 && net.latency_ms <= 50.0, "Normal simulated latency within valid bounds");
    
    Transceiver xcvr;
    transceiver_init(&xcvr, "SN-1", "TEST", TRANSCEIVER_SFP_PLUS, 1310.0);
    transceiver_simulate_telemetry(&xcvr, SCENARIO_CRITICAL);
    TEST_ASSERT(xcvr.is_simulated == true, "Transceiver metrics explicitly marked as simulated");
    TEST_ASSERT(xcvr.rx_power_dbm <= -20.0, "Critical simulated RX power accurately models degraded link");
}

int main(void) {
    printf("====================================================\n");
    printf("     NetDiag-C Diagnostic Engine Unit Test Suite    \n");
    printf("====================================================\n");
    
    test_normal_scenario_diagnostics();
    test_warning_scenario_diagnostics();
    test_critical_scenario_diagnostics();
    test_threshold_config_loader();
    test_device_registry_and_parser();
    test_simulation_bounds_and_disclaimer();
    
    printf("\n====================================================\n");
    printf("Test Results: %d / %d tests passed (%.1f%%)\n",
           g_tests_passed, g_tests_run, (100.0 * g_tests_passed) / (g_tests_run ? g_tests_run : 1));
    printf("====================================================\n");
    
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
