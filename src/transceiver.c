#include "transceiver.h"
#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

void transceiver_init(Transceiver *xcvr, const char *serial, const char *part_no, 
                     TransceiverFormFactor form_factor, double wavelength_nm) {
    if (!xcvr) return;
    
    snprintf(xcvr->serial_number, sizeof(xcvr->serial_number), "%s", serial ? serial : "SN-SIM-0001");
    snprintf(xcvr->vendor_part_number, sizeof(xcvr->vendor_part_number), "%s", part_no ? part_no : "GENERIC-OPT-10G");
    xcvr->form_factor = form_factor;
    xcvr->wavelength_nm = (wavelength_nm > 0.0) ? wavelength_nm : 1310.0;
    
    /* Nominal baseline optical parameters */
    xcvr->rx_power_dbm = -8.5;
    xcvr->tx_power_dbm = -2.5;
    xcvr->laser_bias_current_ma = 32.0;
    xcvr->temperature_c = 38.0;
    xcvr->voltage_v = 3.30;
    xcvr->is_simulated = true;
}

void transceiver_simulate_telemetry(Transceiver *xcvr, ScenarioType scenario) {
    if (!xcvr) return;
    
    /* Explicitly mark telemetry as synthetic */
    xcvr->is_simulated = true;
    
    switch (scenario) {
        case SCENARIO_NORMAL:
            /*
             * Normal optical DDM values:
             * RX Power: -14.0 dBm to -6.0 dBm (good link margin)
             * TX Power: -4.0 dBm to -1.0 dBm
             * Laser Bias Current: 25.0 mA to 40.0 mA (healthy laser diode)
             * Temperature: 30.0 C to 50.0 C
             * Supply Voltage: 3.25 V to 3.35 V (within +/-5% nominal 3.3V)
             */
            xcvr->rx_power_dbm = utils_random_double(-14.0, -6.0);
            xcvr->tx_power_dbm = utils_random_double(-4.0, -1.0);
            xcvr->laser_bias_current_ma = utils_random_double(25.0, 40.0);
            xcvr->temperature_c = utils_random_double(30.0, 50.0);
            xcvr->voltage_v = utils_random_double(3.25, 3.35);
            break;
            
        case SCENARIO_WARNING:
            /*
             * Warning optical DDM conditions:
             * Slight fiber attenuation (dirty connector / macro-bend) or elevated laser bias / temp
             */
            xcvr->rx_power_dbm = utils_random_double(-19.5, -17.1); /* Marginal optical power */
            xcvr->tx_power_dbm = utils_random_double(-7.5, -5.5);
            xcvr->laser_bias_current_ma = utils_random_double(55.0, 68.0); /* Aging laser requiring higher bias */
            xcvr->temperature_c = utils_random_double(65.0, 72.0);        /* Elevated case temperature */
            xcvr->voltage_v = utils_random_double(3.14, 3.19);
            break;
            
        case SCENARIO_CRITICAL:
            /*
             * Critical optical failure:
             * Severe fiber attenuation/break, laser diode end-of-life, or thermal runaway
             */
            xcvr->rx_power_dbm = utils_random_double(-35.0, -25.0); /* Optical signal loss / extreme loss */
            xcvr->tx_power_dbm = utils_random_double(-15.0, -10.0); /* Dying transmitter */
            xcvr->laser_bias_current_ma = utils_random_double(82.0, 110.0); /* Severe bias current saturation */
            xcvr->temperature_c = utils_random_double(78.0, 92.0);          /* Thermal emergency */
            xcvr->voltage_v = utils_random_double(2.90, 3.05);          /* Undervoltage rail fault */
            break;
            
        case SCENARIO_FAULT_SPIKE:
            /* Intermittent optical receiver oscillation / optical overload */
            xcvr->rx_power_dbm = utils_random_double(1.5, 4.0); /* Optical receiver overload risk (>0 dBm) */
            xcvr->tx_power_dbm = utils_random_double(-2.0, 0.5);
            xcvr->laser_bias_current_ma = utils_random_double(75.0, 90.0);
            xcvr->temperature_c = utils_random_double(70.0, 80.0);
            xcvr->voltage_v = utils_random_double(3.55, 3.70); /* Overvoltage */
            break;
    }
}

int transceiver_collect_telemetry(Transceiver *xcvr, const char *bus_or_interface, bool simulate) {
    if (!xcvr) return -1;
    
    if (simulate) {
        int roll = utils_random_int(1, 100);
        if (roll <= 80) {
            transceiver_simulate_telemetry(xcvr, SCENARIO_NORMAL);
        } else if (roll <= 95) {
            transceiver_simulate_telemetry(xcvr, SCENARIO_WARNING);
        } else {
            transceiver_simulate_telemetry(xcvr, SCENARIO_CRITICAL);
        }
        return 0;
    }
    
    /*
     * Future Hardware Integration Hook:
     * When physical I2C DOM sensors or Linux ethtool EEPROM dumps are hooked in,
     * this section will interface with /dev/i2c-X or ioctl(SIOCETHTOOL, ETHTOOL_GMODULEEEPROM).
     */
    LOG_WARN("Physical I2C optical DDM driver not attached for interface '%s'. Using synthetic telemetry.",
             bus_or_interface ? bus_or_interface : "default");
    transceiver_simulate_telemetry(xcvr, SCENARIO_NORMAL);
    xcvr->is_simulated = true;
    return 0;
}

const char *transceiver_form_factor_to_string(TransceiverFormFactor form_factor) {
    switch (form_factor) {
        case TRANSCEIVER_SFP:      return "SFP (1G/2.5G)";
        case TRANSCEIVER_SFP_PLUS: return "SFP+ (10G)";
        case TRANSCEIVER_QSFP28:   return "QSFP28 (100G)";
        case TRANSCEIVER_XFP:      return "XFP (10G)";
        default:                   return "UNKNOWN";
    }
}

TransceiverFormFactor transceiver_parse_form_factor(const char *str) {
    if (!str) return TRANSCEIVER_UNKNOWN;
    
    if (strcasecmp(str, "SFP") == 0) return TRANSCEIVER_SFP;
    if (strcasecmp(str, "SFP+") == 0 || strcasecmp(str, "SFP_PLUS") == 0) return TRANSCEIVER_SFP_PLUS;
    if (strcasecmp(str, "QSFP28") == 0 || strcasecmp(str, "QSFP") == 0) return TRANSCEIVER_QSFP28;
    if (strcasecmp(str, "XFP") == 0) return TRANSCEIVER_XFP;
    
    return TRANSCEIVER_UNKNOWN;
}
