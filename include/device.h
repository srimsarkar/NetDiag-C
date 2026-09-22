#ifndef NETDIAG_DEVICE_H
#define NETDIAG_DEVICE_H

#include <stddef.h>
#include <stdbool.h>
#include "network.h"
#include "transceiver.h"
#include "diagnostics.h"

#define MAX_DEVICES 64
#define DEVICE_ID_LEN 32
#define DEVICE_NAME_LEN 64
#define DEVICE_IP_LEN 46

/**
 * Categorization of network hardware elements.
 */
typedef enum {
    DEVICE_TYPE_ROUTER = 0,
    DEVICE_TYPE_SWITCH,
    DEVICE_TYPE_OPTICAL_TERMINAL,
    DEVICE_TYPE_GATEWAY,
    DEVICE_TYPE_SERVER,
    DEVICE_TYPE_UNKNOWN
} DeviceType;

/**
 * Complete Device entity model.
 * Holds device metadata, dynamic network telemetry, optical transceiver telemetry,
 * and current diagnostic state.
 */
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

/**
 * Registry container managing a collection of monitored network devices.
 */
typedef struct {
    Device devices[MAX_DEVICES];
    size_t count;
} DeviceRegistry;

/**
 * Initializes a device registry container.
 */
void device_registry_init(DeviceRegistry *registry);

/**
 * Loads device inventory from a CSV file into the registry.
 *
 * @param registry Target registry.
 * @param csv_path Path to devices CSV file.
 * @return Number of devices successfully loaded, or negative on error.
 */
int device_registry_load_csv(DeviceRegistry *registry, const char *csv_path);

/**
 * Adds a new device to the registry.
 *
 * @return Pointer to newly added Device in registry, or NULL if full.
 */
Device *device_registry_add(DeviceRegistry *registry, const char *id, const char *name, 
                           const char *ip, DeviceType type, bool has_transceiver);

/**
 * Finds a device by ID.
 *
 * @return Pointer to Device, or NULL if not found.
 */
Device *device_registry_find_by_id(DeviceRegistry *registry, const char *id);

/**
 * Converts a DeviceType enum to a human-readable string.
 */
const char *device_type_to_string(DeviceType type);

/**
 * Parses a device type string to its corresponding enum value.
 */
DeviceType device_parse_type(const char *str);

#endif /* NETDIAG_DEVICE_H */
