# NetDiag-C – C-Based Network Device Monitoring & Diagnostic System

> **A modular, high-performance C system for proactive network health evaluation, optical transceiver digital diagnostic monitoring (DDM) simulation, and rule-based fault remediation.**

---

## 1. Problem Statement

Modern enterprise and telecommunication infrastructures rely heavily on high-speed fiber-optic links (10GbE SFP+, 100GbE QSFP28) and multi-tier routing architectures. When network degradation occurs—such as packet loss, intermittent latency spikes, or CRC framing errors—traditional network tools (e.g., standard ping or traceroute) only detect layer-3 symptom-level failures. They lack physical-layer awareness, often missing underlying optical degradation such as fiber macro-bends, connector contamination, photodiode overload, or laser diode aging.

Furthermore, setting up physical optical testbeds and IoT hardware benches for development and diagnostics validation is cost-prohibitive and inaccessible for many engineers.

---

## 2. Project Objective

**NetDiag-C** bridges the gap between physical-layer telemetry and layer-3 network performance. It provides a modular, lightweight C99-based diagnostic engine that ingests network metrics and optical transceiver telemetry, evaluates them against configurable tolerance thresholds, correlates anomalies across layers, and generates root-cause diagnostic assessments with actionable remediation procedures.

---

## 3. Unique Selling Proposition (USP)

- **Cross-Layer Telemetry Correlation**: Correlates standard network performance metrics (RTT latency, jitter, packet loss, bandwidth utilization, CRC errors) directly with optical Digital Diagnostic Monitoring (DDM / DOM) parameters (RX/TX optical power, laser bias current, case temperature, supply voltage).
- **Rule-Based Root Cause Diagnosis**: Moves beyond passive metric dashboards by classifying health states (`HEALTHY`, `WARNING`, `CRITICAL`) and generating specific, actionable engineering runbook recommendations (e.g., optical ferrule cleaning, laser end-of-life replacement scheduling, thermal mitigation).
- **Zero-Hardware Barrier to Entry**: Features a built-in synthetic telemetry generator that simulates normal, degraded, critical, and transient fault scenarios out of the box, allowing full testing on any standard workstation without physical IoT hardware.
- **Hardware-Ready Extensibility**: Designed with strict separation of concerns, providing pluggable driver interfaces to swap synthetic telemetry for live Linux kernel sockets, `sysfs`, `ethtool`, or physical `/dev/i2c-*` optical DOM EEPROM readers in subsequent hardware releases.

---

## 4. Why NetDiag-C Differs from Generic Network Monitoring

| Feature | Generic Network Monitoring (e.g., Basic Ping/SNMP tools) | NetDiag-C |
|---|---|---|
| **Layer Coverage** | Primarily Layer 3 / Layer 4 (Ping, Port availability) | Physical Layer (Optical DDM) + Layer 2/3 (Latency, Jitter, CRC errors, Loss) |
| **Optical Awareness** | Rarely monitors optical power levels or laser diode bias current | Models SFF-8472 & SFF-8636 digital optical diagnostic metrics |
| **Output Type** | Raw numerical counters and time-series graphs | Structured health classification + actionable remediation steps |
| **Hardware Dependency** | Requires live network interfaces and deployed infrastructure | Zero IoT hardware required for development; includes multi-scenario simulation engine |
| **Architecture** | Often monolithic or heavy daemon-based | Ultra-lightweight, modular C99 with isolated core library |

---

## 5. Key Features

- **Modular C Architecture**: Strict separation of concerns between data collection/simulation, device registry management, diagnostic rule evaluation, monitoring orchestration, and structured logging.
- **Optical DDM / DOM Telemetry Simulation**: Synthesizes realistic optical transceiver metrics modeled after SFF-8472 (SFP/SFP+) and SFF-8636 (QSFP28) standards (RX/TX power in dBm, laser bias current in mA, internal temperature in °C, supply voltage in V).
- **Comprehensive Network Metrics**: Evaluates round-trip time (RTT) latency, jitter, packet drop rate, link bandwidth saturation, throughput, and FCS/CRC error frames.
- **Configurable Diagnostic Thresholds**: Operational boundaries are fully decoupled into external configuration files (`config/thresholds.conf`), allowing operators to define project- or vendor-specific limits.
- **Multi-Level Structured Logger**: Dual-output logging (colorized ANSI console output and persistent file logging to `logs/netdiag.log`) with configurable verbosity levels (`DEBUG`, `INFO`, `WARN`, `ERROR`, `CRITICAL`).
- **Dynamic Scenario Verification**: Built-in test matrix demonstrating nominal operations (`NORMAL`), marginal fiber loss (`WARNING`), severe laser degradation (`CRITICAL`), and optical power overload (`FAULT_SPIKE`).
- **Dual Build System**: Native support for compilation via standard GNU Make and CMake across macOS and Linux.

---

## 6. Telemetry Simulation & Threshold Transparency Notice

> [!IMPORTANT]
> **Simulation Notice (Version 1.0)**:
> In this release of NetDiag-C, all optical measurements (RX/TX optical power, laser bias current, internal temperature, voltage) and network metrics are **synthetically generated** by the simulation engine. They do **not** represent direct physical readings from live I2C/DOM hardware sensors. Both `NetworkStats` and `Transceiver` data structures include an explicit `is_simulated = true` flag. Version 1 does not require IoT hardware.
>
> **Configurable Thresholds Notice**:
> The diagnostic boundaries defined in `config/thresholds.conf` represent project-defined baseline tolerances for testing and demonstration purposes. They are **not** universal industry thresholds. Operating tolerances should be tailored to specific transceiver vendor datasheets and network SLA requirements in production deployments.

---

## 7. System Architecture

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

## 8. Diagnostic Workflow

NetDiag-C operates on a six-stage diagnostic pipeline:

```
[ MONITOR ]  -->  [ DETECT ]  -->  [ CORRELATE ]  -->  [ DIAGNOSE ]  -->  [ RECOMMEND ]  -->  [ LOG ]
```

1. **Monitor**: The Monitoring Engine polls telemetry from registered network appliances and optical modules (via simulation or hardware driver hooks).
2. **Detect**: Telemetry metrics are evaluated against project-configured warning and critical thresholds.
3. **Correlate**: Cross-layer indicators are analyzed concurrently (e.g., high packet loss correlated with optical RX power attenuation).
4. **Diagnose**: An aggregate health state (`OK`, `WARNING`, `CRITICAL`) is assigned with specific diagnostic fault codes.
5. **Recommend**: The engine looks up remediation procedures and outputs targeted engineering guidance.
6. **Log**: Events, diagnostic codes, and recommendations are recorded in the structured logger and rendered in the CLI dashboard.

---

## 9. Module Structure

```
NetDiag-C/
├── CMakeLists.txt              # CMake build configuration
├── Makefile                    # GNU Make build automation
├── README.md                   # Project documentation
├── .gitignore                  # Git ignore rules
├── config/
│   └── thresholds.conf         # Configurable threshold definitions
├── data/
│   └── sample_devices.csv      # CSV device inventory database
├── docs/
│   ├── architecture.md         # In-depth architectural design specification
│   └── troubleshooting.md      # Diagnostic code reference & runbook
├── include/                    # Public C header definitions
│   ├── device.h                # Device model & inventory management
│   ├── diagnostics.h           # Threshold structures & diagnostic rule engine
│   ├── logger.h                # Structured console/file logger interface
│   ├── monitoring.h            # Monitoring engine & dashboard renderer
│   ├── network.h               # Network telemetry structures & simulation
│   ├── transceiver.h           # Optical transceiver DDM structures & simulation
│   └── utils.h                 # String parsing, CSV, and timing utilities
├── logs/                       # Application runtime logs
│   └── netdiag.log
├── src/                        # Core implementation sources
│   ├── device.c                # Device registry & CSV parser implementation
│   ├── diagnostics.c           # Rule-based threshold evaluation engine
│   ├── logger.c                # ANSI color console & file logger
│   ├── main.c                  # CLI entrypoint & argument dispatcher
│   ├── monitoring.c            # Monitoring coordinator & table formatter
│   ├── network.c               # Network performance simulation & driver hook
│   ├── transceiver.c           # Optical DDM telemetry simulation & driver hook
│   └── utils.c                 # Utility helper functions
└── tests/
    └── test_diagnostics.c      # Automated unit test suite (31 test cases)
```

---

## 10. Technologies Used

- **Language**: C (C99 / C11 standard compliant)
- **Standard Libraries**: Standard C Library (`stdio.h`, `stdlib.h`, `string.h`, `time.h`, `math.h`), POSIX API (`unistd.h`, `sys/stat.h`)
- **Build Automation**: GNU Make (`cc` / `gcc` / `clang`), CMake (3.10+)
- **Testing**: Native C assertion-based test runner with structured reporting
- **Target Environments**: macOS (Apple Silicon / Intel), Linux (Ubuntu, Debian, RHEL, Alpine)

---

## 11. Installation Requirements

### Prerequisites
- **C Compiler**: `gcc` (v4.9+) or `clang` (Apple Clang or LLVM Clang)
- **Build Tools**: `make` (GNU Make) or `cmake` (3.10+)
- **Version Control**: `git`

---

## 12. Build Instructions for macOS & Linux

### Using GNU Make (Recommended)

```bash
# Clone the repository
git clone https://github.com/srimsarkar/NetDiag-C.git
cd NetDiag-C

# Compile the application and test suite
make

# Run the automated unit test suite
make test
```

### Using CMake

```bash
# Generate build files
mkdir -p build && cd build
cmake ..

# Build targets
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
cd ..
```

---

## 13. How to Run the Application

```bash
# Run 4-stage scenario demonstration (Normal, Warning, Critical, Fault Spike)
./build/bin/netdiag --demo

# Run a single monitoring cycle across the device inventory
./build/bin/netdiag --single

# Run periodic monitoring (e.g., 5 cycles at 2-second intervals)
./build/bin/netdiag --cycles 5 --interval 2

# Inspect a specific device deep-dive report
./build/bin/netdiag --single --detail DEV-OLT-METRO1

# Run with custom threshold configuration and device inventory
./build/bin/netdiag --config config/thresholds.conf --devices data/sample_devices.csv --verbose
```

### CLI Command Options

| Option | Argument | Description |
|---|---|---|
| `--demo` | None | Executes the automated 4-stage scenario demonstration matrix |
| `--single`, `-s` | None | Runs a single monitoring cycle and displays summary + warnings |
| `--cycles` | `<N>` | Runs N periodic monitoring cycles (default: 1) |
| `--interval` | `<sec>` | Sleep duration between cycles in seconds (default: 3) |
| `--config` | `<path>` | Custom path to threshold configuration file (default: `config/thresholds.conf`) |
| `--devices` | `<path>` | Custom path to device CSV inventory (default: `data/sample_devices.csv`) |
| `--detail` | `<id>` | Displays full telemetry and diagnostic deep-dive for a specific device ID |
| `--log` | `<path>` | Custom destination path for the execution log file (default: `logs/netdiag.log`) |
| `--verbose`, `-v` | None | Enables verbose `DEBUG` level log output |
| `--help`, `-h` | None | Prints the CLI usage manual |

---

## 14. Example Terminal Output

### Summary Dashboard Table
```
+-------------------------------------------------------------------------------------------------------------------------------+
| Device ID    | Name                 | IP Address      | RTT Lat  | Pkt Loss | Opt RX Pwr  | Opt Temp   | Status         |
+-------------------------------------------------------------------------------------------------------------------------------+
| DEV-RTR-CORE01 | Core Border Gateway  | 10.0.0.1        |   14.2ms |   0.00% | -8.5dBm     | 38.2°C    | OK             |
| DEV-RTR-EDGE02 | Edge Aggregation Rou | 10.0.0.2        |   72.4ms |   3.15% | -18.2dBm    | 41.0°C    | WARNING        |
| DEV-SW-DIST01 | Distribution Spine S | 10.0.1.1        |    8.1ms |   0.02% | -9.2dBm     | 45.1°C    | OK             |
| DEV-OLT-METRO1 | Metro Optical Line T | 10.0.2.1        |  244.6ms |  26.43% | -30.6dBm    | 88.1°C    | CRITICAL       |
| DEV-SRV-APP01 | High-Performance Com | 10.0.4.10       |    6.5ms |   0.00% | N/A         | N/A        | OK             |
+-------------------------------------------------------------------------------------------------------------------------------+
```

### Detailed Diagnostic Deep-Dive (`DEV-OLT-METRO1`)
```
[Device Metadata]
  ID:           DEV-OLT-METRO1
  Name:         Metro Optical Line Terminal
  IP Address:   10.0.2.1
  Type:         OPTICAL_TERMINAL
  Status:       CRITICAL
  Last Checked: 2026-09-23 00:35:08

[Network Performance Telemetry] (Simulated: YES)
  Latency (RTT):         244.60 ms
  Jitter:                 48.20 ms
  Packet Loss:            26.43 %
  Bandwidth Utilization:  98.2 %
  Throughput:            145.0 Mbps
  Error Frames (CRC):   1420 frames

[Optical Transceiver DDM Telemetry] (Simulated: YES (SFF-8472 Model))
  Serial Number:        OPT-XFP-3320
  Part Number:          XFP-10G-ER
  Form Factor:          XFP (10G)
  Wavelength:           1550.0 nm
  RX Optical Power:     -30.60 dBm
  TX Optical Power:     -10.31 dBm
  Laser Bias Current:    93.99 mA
  Internal Temperature:   88.1 °C
  Supply Voltage:         2.98 V

[Diagnostic Evaluation & Remediation Guidance]
  Detected Anomalies (5):
    [CRITICAL] [OPT_RX_PWR_CRIT_LOW] Optical RX power severely degraded (-30.60 dBm <= critical low -22.00 dBm)
    [CRITICAL] [OPT_TX_PWR_CRIT_LOW] Optical TX power dying (-10.31 dBm <= critical low -10.00 dBm)
    [CRITICAL] [OPT_LASER_BIAS_CRIT] Laser bias current saturation (94.0 mA >= 80.0 mA)
    [CRITICAL] [OPT_TEMP_CRITICAL] Transceiver thermal emergency (88.1 C >= 75.0 C)
    [CRITICAL] [OPT_VOLTAGE_FAULT] Transceiver supply voltage out of bounds (2.98 V)
  Recommended Corrective Actions (5):
    → Check for severe fiber macro-bends, dirty LC connectors, or upstream laser failure.
    → Transmitter laser diode failure imminent. Schedule transceiver replacement.
    → Laser diode is aging out of spec; replace optical module S/N OPT-XFP-3320.
    → Check chassis fan tray operation, air filters, and rack ambient cooling.
    → Check switch line-card power rail regulators.
```

---

## 15. Test Results

NetDiag-C includes an automated C unit test suite ([test_diagnostics.c](file:///Users/sreyankasarkar/NetDiag-C/tests/test_diagnostics.c)) verifying all core functions:

```
====================================================
     NetDiag-C Diagnostic Engine Unit Test Suite    
====================================================

--- Running Test: Normal Scenario Diagnostics ---
  [PASS] Overall status should be OK
  [PASS] Issue count should be 0
  [PASS] Recommendation should provide confirmation

--- Running Test: Warning Scenario Diagnostics ---
  [PASS] Elevated latency triggers WARNING status
  [PASS] Detected 1 network warning issue
  [PASS] Issue code matches NET_LATENCY_WARN
  [PASS] Marginal optical RX power triggers WARNING status
  [PASS] Detected 1 optical warning issue
  [PASS] Issue code matches OPT_RX_PWR_WARN_LOW

--- Running Test: Critical Scenario Diagnostics ---
  [PASS] Severe metrics trigger CRITICAL status
  [PASS] Multiple critical issues identified
  [PASS] Multiple actionable recommendations generated

--- Running Test: Threshold Config Loader ---
  [PASS] Threshold config loads successfully
  [PASS] Latency warning threshold parsed
  [PASS] Optical RX power threshold parsed

--- Running Test: Device Registry & Parsing ---
  [PASS] Registry starts empty
  [PASS] Device added to registry
  [PASS] Registry count is 1
  [PASS] Find by ID retrieves device
  [PASS] Parser maps ROUTER correctly
  [PASS] Parser maps SWITCH correctly
  [PASS] Parser maps QSFP28 correctly
  [PASS] CSV parser extracts all 9 tokens including trailing empty fields
  [PASS] First token is valid
  [PASS] Fifth token is valid
  [PASS] Trailing empty token is an empty string
  [PASS] utils_ensure_parent_dir successfully creates parent directories

--- Running Test: Simulation Bounds & Transparency ---
  [PASS] Network stats explicitly marked as simulated
  [PASS] Normal simulated latency within valid bounds
  [PASS] Transceiver metrics explicitly marked as simulated
  [PASS] Critical simulated RX power accurately models degraded link

====================================================
Test Results: 31 / 31 tests passed (100.0%)
====================================================
```

---

## 16. Current Limitations

1. **Synthetic Telemetry**: Telemetry is synthetically simulated via mathematical models rather than polled from live physical I2C busses or kernel netlink sockets.
2. **Project-Defined Thresholds**: Baseline thresholds in `thresholds.conf` are reference values for evaluation and must be adapted to individual vendor specifications.
3. **No Embedded Web UI**: Output is currently rendered in the terminal CLI and structured log files; a graphical interface or web frontend is not included in Version 1.

---

## 17. Future Work

- [ ] **Physical I2C / DDM Driver**: Implement direct optical EEPROM reading via Linux `/dev/i2c-*` (SFF-8472 memory map addresses `0xA0` and `0xA2`).
- [ ] **Kernel-Level Network Telemetry**: Integrate raw socket ICMP probing and `AF_PACKET` / `ethtool` ioctl interface metrics.
- [ ] **OpenTelemetry & Prometheus Exporter**: Expose diagnostic metrics via an embedded HTTP endpoint (`/metrics`) for Grafana scraping.
- [ ] **eBPF Link Tracing**: Integrate eBPF programs for high-resolution per-packet microburst and drop analysis.

---

## 18. Relevance to Networking & Device Troubleshooting

In high-reliability enterprise networks and Internet Service Provider (ISP) backbones, diagnosing root causes quickly reduces Mean Time to Resolution (MTTR) and prevents cascading service outages. 

NetDiag-C demonstrates practical mastery of:
- **Systems Programming in C**: Clean memory management, POSIX file I/O, string tokenization, structured error handling, and robust CLI design.
- **Telecom & Optical Standards**: Practical understanding of Digital Diagnostic Monitoring (DDM / DOM), optical link budgets, laser diode physics, and form factor specifications (SFP+, QSFP28, XFP).
- **Network Troubleshooting Methodologies**: Correlating physical-layer degradation (attenuation, optical overload, laser aging) with upper-layer transport anomalies (CRC errors, packet loss, jitter).
