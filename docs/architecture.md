# NetDiag-C System Architecture

**NetDiag-C** is a modular C-based Network Device Monitoring and Diagnostic System designed for enterprise and telecom environments. It monitors network link performance and optical transceiver Digital Diagnostic Monitoring (DDM / DOM) telemetry, detects operational anomalies against configurable thresholds, and generates actionable remediation recommendations.

---

## 1. High-Level Modular Architecture

```
+-----------------------------------------------------------------------------+
|                                    CLI / UI                                 |
|                                 (src/main.c)                                |
+-----------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------+
|                              Monitoring Engine                              |
|                             (src/monitoring.c)                              |
+-----------------------------------------------------------------------------+
         |                             |                              |
         v                             v                              v
+------------------+         +-------------------+          +------------------+
| Device Registry  |         | Telemetry Driver  |          | Diagnostic Engine|
|  (src/device.c)  |         |      Layer        |          |(src/diagnostics.c|
+------------------+         +-------------------+          +------------------+
                             /                 \                      |
                            v                   v                     v
                 +-------------------+ +-------------------+ +-----------------+
                 | Network Telemetry | | Optical DDM Telem | | Threshold Config|
                 |  (src/network.c)  | |(src/transceiver.c)| |(thresholds.conf)|
                 +-------------------+ +-------------------+ +-----------------+
                            |                   |
                            v                   v
                 +-------------------+ +-------------------+
                 | Synthetic Engine  | | Synthetic Engine  |
                 | (Simulated Model) | | (SFF-8472 Model)  |
                 +-------------------+ +-------------------+
                            |                   |
                            :                   :
                 [Future Extensibility Hook]    [Future Extensibility Hook]
                 Linux Sockets / sysfs / ethtool    Linux I2C Bus (/dev/i2c-*)
```

---

## 2. Component Breakdown

### 2.1 Core Subsystems

| Module | Files | Responsibility |
|---|---|---|
| **Monitoring Engine** | [`monitoring.c`](file:///Users/sreyankasarkar/NetDiag-C/src/monitoring.c), [`monitoring.h`](file:///Users/sreyankasarkar/NetDiag-C/include/monitoring.h) | Orchestrates telemetry polling cycles, coordinates diagnostics, manages execution modes (single cycle, demo scenario matrix, continuous loop). |
| **Device Inventory** | [`device.c`](file:///Users/sreyankasarkar/NetDiag-C/src/device.c), [`device.h`](file:///Users/sreyankasarkar/NetDiag-C/include/device.h) | Device inventory management, CSV parser, state persistence, device type classification. |
| **Network Telemetry** | [`network.c`](file:///Users/sreyankasarkar/NetDiag-C/src/network.c), [`network.h`](file:///Users/sreyankasarkar/NetDiag-C/include/network.h) | Network performance metrics (latency, jitter, packet loss, bandwidth utilization, CRC errors). |
| **Optical Transceiver Telemetry** | [`transceiver.c`](file:///Users/sreyankasarkar/NetDiag-C/src/transceiver.c), [`transceiver.h`](file:///Users/sreyankasarkar/NetDiag-C/include/transceiver.h) | Optical DDM/DOM telemetry (RX/TX optical power, laser bias current, case temperature, supply voltage). |
| **Diagnostic Rule Engine** | [`diagnostics.c`](file:///Users/sreyankasarkar/NetDiag-C/src/diagnostics.c), [`diagnostics.h`](file:///Users/sreyankasarkar/NetDiag-C/include/diagnostics.h) | Multi-parameter threshold comparison, fault identification, classification (OK, WARNING, CRITICAL), root-cause remediation generator. |
| **Structured Logger** | [`logger.c`](file:///Users/sreyankasarkar/NetDiag-C/src/logger.c), [`logger.h`](file:///Users/sreyankasarkar/NetDiag-C/include/logger.h) | Multi-level logging (DEBUG, INFO, WARN, ERROR, CRITICAL) to console with ANSI colors and disk log file (`logs/netdiag.log`). |
| **Utilities** | [`utils.c`](file:///Users/sreyankasarkar/NetDiag-C/src/utils.c), [`utils.h`](file:///Users/sreyankasarkar/NetDiag-C/include/utils.h) | String tokenizers, CSV splitting, random bounded distributions, date-time formatting. |

---

## 3. Data Model & Key Structures

### 3.1 Device Model
```c
typedef struct Device {
    char id[DEVICE_ID_LEN];
    char name[DEVICE_NAME_LEN];
    char ip_address[DEVICE_IP_LEN];
    DeviceType type;
    
    bool has_transceiver;
    NetworkStats network_stats;
    Transceiver transceiver;
    DiagnosticResult diagnostic_result;
    DeviceStatus status;
    char last_updated[64];
} Device;
```

### 3.2 Network Telemetry (`NetworkStats`)
```c
typedef struct {
    double latency_ms;
    double jitter_ms;
    double packet_loss_pct;
    double bandwidth_util_pct;
    double throughput_mbps;
    uint64_t error_frames;
    bool is_simulated;
} NetworkStats;
```

### 3.3 Optical Transceiver Telemetry (`Transceiver`)
Compliant with SFF-8472 (SFP/SFP+) and SFF-8636 (QSFP28):
```c
typedef struct {
    char serial_number[32];
    char vendor_part_number[32];
    TransceiverFormFactor form_factor;
    double wavelength_nm;
    
    double rx_power_dbm;
    double tx_power_dbm;
    double laser_bias_current_ma;
    double temperature_c;
    double voltage_v;
    
    bool is_simulated;
} Transceiver;
```

---

## 4. Telemetry Simulation & Physical Driver Extensibility

### 4.1 Telemetry Transparency Notice
> [!IMPORTANT]
> In this release of NetDiag-C, all optical measurements and network performance values are generated by synthetic telemetry simulation engines. They are **NOT** direct readings from physical hardware sensors. Both `NetworkStats` and `Transceiver` structures maintain an explicit `is_simulated = true` flag.

### 4.2 Future Physical Hardware Driver Integration
The architecture decouples collection from diagnostics:
```c
int network_collect_telemetry(NetworkStats *stats, const char *interface_or_host, bool simulate);
int transceiver_collect_telemetry(Transceiver *xcvr, const char *bus_or_interface, bool simulate);
```

When integrating physical hardware:
1. **Network Driver**: Implement raw socket ICMP probing, Linux netlink/sysfs `/sys/class/net/<iface>/statistics`, or `ethtool` ioctl calls.
2. **Optical Transceiver Driver**: Implement Linux I2C device bus access `/dev/i2c-X` querying address `0x50` (A0h) for identification and `0x51` (A2h) for DDM digital optical monitoring registers as defined in SFF-8472.
