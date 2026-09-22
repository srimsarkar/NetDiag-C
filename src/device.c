#include "device.h"
#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void device_registry_init(DeviceRegistry *registry) {
    if (!registry) return;
    registry->count = 0;
    memset(registry->devices, 0, sizeof(registry->devices));
}

Device *device_registry_add(DeviceRegistry *registry, const char *id, const char *name, 
                           const char *ip, DeviceType type, bool has_transceiver) {
    if (!registry || !id || registry->count >= MAX_DEVICES) {
        return NULL;
    }
    
    /* Check for duplicate ID */
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->devices[i].id, id) == 0) {
            LOG_WARN("Device with ID '%s' already exists in registry.", id);
            return &registry->devices[i];
        }
    }
    
    Device *dev = &registry->devices[registry->count++];
    memset(dev, 0, sizeof(Device));
    
    snprintf(dev->id, sizeof(dev->id), "%s", id);
    snprintf(dev->name, sizeof(dev->name), "%s", name ? name : "Unnamed Device");
    snprintf(dev->ip_address, sizeof(dev->ip_address), "%s", ip ? ip : "0.0.0.0");
    dev->type = type;
    dev->has_transceiver = has_transceiver;
    dev->status = DEVICE_STATUS_UNKNOWN;
    
    network_stats_init(&dev->network_stats);
    if (has_transceiver) {
        transceiver_init(&dev->transceiver, NULL, NULL, TRANSCEIVER_SFP_PLUS, 1310.0);
    }
    
    utils_get_timestamp(dev->last_updated, sizeof(dev->last_updated));
    return dev;
}

Device *device_registry_find_by_id(DeviceRegistry *registry, const char *id) {
    if (!registry || !id) return NULL;
    
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->devices[i].id, id) == 0) {
            return &registry->devices[i];
        }
    }
    return NULL;
}

int device_registry_load_csv(DeviceRegistry *registry, const char *csv_path) {
    if (!registry || !csv_path) return -1;
    
    FILE *fp = fopen(csv_path, "r");
    if (!fp) {
        LOG_ERROR("Unable to open device inventory CSV file: '%s'", csv_path);
        return -1;
    }
    
    char line[512];
    int loaded = 0;
    int line_num = 0;
    
    /* CSV format: id,name,ip_address,type,has_transceiver,xcvr_serial,xcvr_part,xcvr_form,wavelength_nm */
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char *trimmed = utils_trim(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue; /* Skip empty lines and comments */
        }
        
        /* Skip header row if present */
        if (line_num == 1 && strstr(trimmed, "id,") != NULL) {
            continue;
        }
        
        char *tokens[10];
        int num_tokens = utils_split_csv_line(trimmed, tokens, 10);
        if (num_tokens < 4) {
            LOG_WARN("Skipping malformed CSV line %d in '%s'", line_num, csv_path);
            continue;
        }
        
        const char *id = tokens[0];
        const char *name = tokens[1];
        const char *ip = tokens[2];
        DeviceType type = device_parse_type(tokens[3]);
        bool has_xcvr = (num_tokens > 4) ? (atoi(tokens[4]) != 0 || strcasecmp(tokens[4], "true") == 0) : false;
        
        Device *dev = device_registry_add(registry, id, name, ip, type, has_xcvr);
        if (dev && has_xcvr && num_tokens >= 6) {
            const char *sn = (num_tokens > 5 && strlen(tokens[5]) > 0) ? tokens[5] : "SN-OPT-DEFAULT";
            const char *part = (num_tokens > 6 && strlen(tokens[6]) > 0) ? tokens[6] : "SFP-10G-LR";
            TransceiverFormFactor ff = (num_tokens > 7) ? transceiver_parse_form_factor(tokens[7]) : TRANSCEIVER_SFP_PLUS;
            double wl = (num_tokens > 8) ? utils_parse_double(tokens[8], 1310.0) : 1310.0;
            
            transceiver_init(&dev->transceiver, sn, part, ff, wl);
        }
        
        if (dev) {
            loaded++;
        }
    }
    
    fclose(fp);
    LOG_INFO("Loaded %d network devices from '%s'", loaded, csv_path);
    return loaded;
}

const char *device_type_to_string(DeviceType type) {
    switch (type) {
        case DEVICE_TYPE_ROUTER:           return "ROUTER";
        case DEVICE_TYPE_SWITCH:           return "SWITCH";
        case DEVICE_TYPE_OPTICAL_TERMINAL: return "OPTICAL_TERMINAL";
        case DEVICE_TYPE_GATEWAY:          return "GATEWAY";
        case DEVICE_TYPE_SERVER:           return "SERVER";
        default:                           return "UNKNOWN";
    }
}

DeviceType device_parse_type(const char *str) {
    if (!str) return DEVICE_TYPE_UNKNOWN;
    
    if (strcasecmp(str, "ROUTER") == 0) return DEVICE_TYPE_ROUTER;
    if (strcasecmp(str, "SWITCH") == 0) return DEVICE_TYPE_SWITCH;
    if (strcasecmp(str, "OPTICAL_TERMINAL") == 0 || strcasecmp(str, "OLT") == 0) return DEVICE_TYPE_OPTICAL_TERMINAL;
    if (strcasecmp(str, "GATEWAY") == 0) return DEVICE_TYPE_GATEWAY;
    if (strcasecmp(str, "SERVER") == 0) return DEVICE_TYPE_SERVER;
    
    return DEVICE_TYPE_UNKNOWN;
}
