# NetDiag-C Troubleshooting & Diagnostic Runbook

This runbook details common network and optical transceiver anomalies detected by NetDiag-C, along with root causes and corrective actions.

---

## 1. Diagnostic Code Reference

| Code | Severity | Description | Probable Root Cause |
|---|---|---|---|
| `NET_LATENCY_WARN` | WARNING | Elevated RTT latency (> 50ms) | Upstream queue buildup, route detour, link congestion |
| `NET_LATENCY_CRIT` | CRITICAL | Extreme RTT latency (> 150ms) | Severe routing flapping, link saturation, satellite/transoceanic failover |
| `NET_PKTLOSS_WARN` | WARNING | Packet loss rate between 1.0% - 5.0% | Tail drop under bursty traffic, QoS misconfiguration |
| `NET_PKTLOSS_CRIT` | CRITICAL | Severe packet loss (> 5.0%) | Physical fiber impairment, duplex mismatch, failing ASIC port |
| `NET_CRC_ERRORS` | CRITICAL | Excessive CRC / FCS error frames | Faulty patch cord, dirty optical connectors, loose transceiver seating |
| `OPT_RX_PWR_WARN_LOW`| WARNING | Optical RX power low (-17 to -22 dBm) | Fiber macro-bend, patch cable stress, dirty connector ferrule |
| `OPT_RX_PWR_CRIT_LOW`| CRITICAL | Optical RX power critical (<= -22 dBm)| Broken fiber strand, disconnected patch lead, upstream transmitter failure |
| `OPT_RX_PWR_OVERLOAD`| CRITICAL | Optical RX power too high (> +2 dBm) | Short-distance patch without required optical attenuator |
| `OPT_TX_PWR_CRIT_LOW`| CRITICAL | Optical TX power dying (<= -10 dBm) | Transmit laser diode failure |
| `OPT_LASER_BIAS_CRIT`| CRITICAL | Laser bias current saturated (>= 80 mA) | Laser diode aging, optical module end-of-life |
| `OPT_TEMP_CRITICAL`  | CRITICAL | Transceiver case temp (>= 75°C) | Blocked air filters, fan tray failure, hot aisle thermal trap |
| `OPT_VOLTAGE_FAULT`  | CRITICAL | Voltage rail out of bounds (< 3.0V / > 3.6V) | Line-card power supply regulator failure |

---

## 2. Standard Remediation Procedures

### 2.1 Optical Power Loss (RX Power Degraded)
1. **Clean Optical End-Faces**: 
   - Use a specialized 1.25mm (LC) or 2.5mm (SC) one-click fiber cleaner on the patch cable ferrules and the transceiver optical bore.
   - Inspect with a fiber microscope for scratches or contamination.
2. **Inspect Fiber Bend Radius**:
   - Verify that single-mode/multi-mode patch cables do not exceed minimum bend radius (< 30mm) in cable trays or rack doors.
3. **Verify Optical Budget**:
   - Check fiber distance and patch panel attenuation against the transceiver optic class (SR: ~300m, LR: ~10km, ER: ~40km).

### 2.2 Elevated Laser Bias Current
- When a semiconductor laser diode degrades over time, the internal automatic power control (APC) circuit increases bias current to maintain optical output.
- **Action**: Schedule a maintenance window to swap out the optical module before sudden link death.

### 2.3 Transceiver Thermal Warning
1. Verify chassis fan tray speeds and airflow status via out-of-band management.
2. Ensure rack blanking panels are properly installed to avoid hot-air recirculation.
3. Clean dust filters on the intake bezel.

---

## 3. Application Logging & Verification

NetDiag-C writes structured runtime event logs to `logs/netdiag.log`.

To inspect live diagnostics:
```bash
tail -f logs/netdiag.log
```

To run an automated health sweep across all registered devices:
```bash
./build/netdiag --single
```
