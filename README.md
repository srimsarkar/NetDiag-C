# NetDiag-C – C-Based Network Device Monitoring & Diagnostic System

**NetDiag-C** is a modular, high-performance C-based monitoring and diagnostic system for enterprise network appliances and optical transceivers. It ingests telemetry metrics, evaluates them against configurable diagnostic thresholds, and produces actionable root-cause recommendations.

---

## Key Features

- **Modular C Architecture**: Strict separation of concerns across device inventory, telemetry collection, diagnostic rules engine, monitoring orchestration, and structured logging.
- **Optical DDM / DOM Telemetry**: Models SFF-8472 and SFF-8636 digital diagnostic parameters (optical RX/TX power in dBm, laser bias current in mA, internal case temperature in °C, supply voltage).
- **Network Performance Monitoring**: Evaluates round-trip latency, jitter, packet loss rate, bandwidth utilization, and framing errors.
- **Rule-Based Diagnostic Engine**: Compares multi-variable metrics against configurable tolerance boundaries to classify health states (`OK`, `WARNING`, `CRITICAL`).
- **Actionable Remediation Guidance**: Generates practical steps (e.g. optical end-face cleaning, fiber bend mitigation, laser end-of-life replacement).
- **Dual Build System**: Compilable with standard `make` or `cmake`.
- **Zero Hardware Requirement**: Includes a synthetic telemetry engine capable of simulating normal, warning, critical, and fault-spike network profiles without needing physical IoT or optical testbeds.

---

## Telemetry Simulation Transparency

> [!NOTE]
> In this release, optical measurements (RX/TX power, laser bias, temperature, voltage) and network link performance statistics are **synthetically simulated** to allow evaluation and testing on any workstation without requiring physical IoT or optical switch hardware. The telemetry provider layer is designed with clean abstraction hooks so physical Linux kernel interfaces (sockets, sysfs, ethtool, and `/dev/i2c-*` optical EEPROM readers) can be attached seamlessly.

---

## Directory Structure

```
NetDiag-C/
├── CMakeLists.txt          # CMake build configuration
├── Makefile                # GNU Make automation
├── README.md               # Project documentation
├── .gitignore              # Git ignore file
├── config/
│   └── thresholds.conf     # Configurable threshold definitions
├── data/
│   └── sample_devices.csv  # Inventory of monitored network devices
├── docs/
│   ├── architecture.md     # Detailed modular architectural design
│   └── troubleshooting.md  # Runbook and diagnostic error code guide
├── include/                # Public header files
│   ├── device.h            # Device data structures and registry
│   ├── diagnostics.h       # Diagnostic engine and result types
│   ├── logger.h            # Multi-level structured logger
│   ├── monitoring.h        # Monitoring engine coordinator
│   ├── network.h           # Network telemetry structures & simulation
│   ├── transceiver.h       # Optical transceiver DDM telemetry
│   └── utils.h             # Utility helpers (parsing, strings, time)
├── logs/                   # Runtime execution log directory
├── src/                    # Source implementation files
│   ├── device.c
│   ├── diagnostics.c
│   ├── logger.c
│   ├── main.c
│   ├── monitoring.c
│   ├── network.c
│   ├── transceiver.c
│   └── utils.c
└── tests/
    └── test_diagnostics.c  # Unit test suite
```

---

## Building and Running

### Method 1: Using GNU Make (Recommended)

```bash
# Build the application and test binaries
make

# Run the automated diagnostic test suite
make test

# Run the 4-stage scenario demo (Normal, Warning, Critical, Fault Spike)
make demo

# Run a single monitoring cycle across sample devices
make run
```

### Method 2: Using CMake

```bash
mkdir -p build && cd build
cmake ..
make

# Run tests
ctest --output-on-failure
# Or directly:
./test_diagnostics

# Run application
./netdiag --demo
cd ..
```

---

## CLI Options

```
Usage: ./build/bin/netdiag [OPTIONS]

Options:
  --demo               Run the scenario matrix (Normal, Warning, Critical, Spike)
  --single             Run a single diagnostic monitoring cycle and display reports
  --cycles <N>         Run N periodic monitoring cycles
  --interval <sec>     Interval between cycles in seconds (default: 3)
  --config <path>      Path to threshold config file (default: config/thresholds.conf)
  --devices <path>     Path to devices CSV file (default: data/sample_devices.csv)
  --log <path>         Path to log file (default: logs/netdiag.log)
  --detail <device_id> Print detailed report for a specific device
  --verbose, -v        Enable verbose DEBUG logging
  --help, -h           Display this help message and exit
```

---

## Test Scenarios Demonstrated

1. **Nominal State (NORMAL)**:
   - Latency: < 20 ms, Packet Loss: 0%, RX Power: -10 dBm, Temp: 38°C -> `Status: OK`
2. **Optical Degradation (WARNING)**:
   - Elevated latency (70 ms) or degraded optical RX power (-18.5 dBm) -> `Status: WARNING`
3. **Laser Saturation & Link Saturation (CRITICAL)**:
   - Severe packet loss (15%), laser bias current saturated (95 mA), high temp (82°C) -> `Status: CRITICAL`
4. **Transient Burst / Optical Overload (FAULT_SPIKE)**:
   - High jitter (120 ms) and RX power overload (+3.2 dBm) -> `Status: CRITICAL`
