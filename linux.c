#if defined(__linux__)
#include "common.h"

int get_sys_os_dist(char **name, char **version) {
    char buf[255] = {0};
    const char *filename = "/etc/os-release";
    FILE *fp;

    fp = fopen(filename, "r");
    if (!fp) {
        *name = strdup("Unknown");
        *version = strdup("Unknown");
        return -1;
    }

    while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
        rstrip(buf);
        char *key, *value;
        char *start;
        start = buf;
        key = strchr(buf, '=');
        if (key) {
            size_t diff = key - start;
            *key = 0;
            key = start;
            value = key + diff + 1;
        } else {
            continue;
        }
        key = buf;

        if (value[0] == '\"') {
            memmove(value, &value[1], strlen(&value[1]));
            if (strlen(value))
                value[strlen(value) - 1] = '\0';
        }
        if (strlen(value) && value[strlen(value) - 1] == '\"') {
            value[strlen(value) - 1] = '\0';
        }
        if (!strcmp(key, "NAME")) {
            (*name) = strdup(value);
        }
        if (!strcmp(key, "VERSION") || !strcmp(key, "VERSION_ID") || !strcmp(key, "BUILD_ID")) {
            (*version) = strdup(value);
        }
    }
    fclose(fp);
    return 0;
}

ssize_t get_sys_memory() {
    char buf[255] = {0};
    ssize_t result;
    FILE *fp;

    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Unable to open /proc/meminfo");
        return -1;
    }

    result = 0;
    while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
        char *key = strstr(buf, "MemTotal:");
        if (key) {
            result = strtoll(key + strlen("MemTotal:"), NULL, 10);
            break;
        }
    }

    fclose(fp);
    return result;
}

char *get_sys_dmi_product() {
    FILE *fp;
    static char buf[255];

    fp = fopen("/sys/class/dmi/id/product_name", "r");
    if (!fp) {
        return NULL;
    }

    if (!fgets(buf, sizeof(buf) - 1, fp)) {
        perror("Unable to read system vendor");
        if (fp != NULL) {
            fclose(fp);
        }
        return NULL;
    }

    fclose(fp);
    return buf;
}

struct Block_Device **get_block_devices(size_t *total) {
    struct Block_Device **result;
    struct dirent *rec;
    DIR *dp;
    size_t i;
    size_t devices_total;

    dp = opendir("/sys/block");
    if (!dp) {
        perror("/sys/block");
        return 0;
    }

    i = 0;
    devices_total = 0;
    *total = devices_total;
    while ((rec = readdir(dp)) != NULL) {
        if (!strcmp(rec->d_name, ".") || !strcmp(rec->d_name, "..")) {
            continue;
        }
        devices_total++;
    }
    rewinddir(dp);

    result = calloc(devices_total + 1, sizeof(*result));
    if (!result) {
        perror("Unable to allocate device array");
        return NULL;
    }

    while ((rec = readdir(dp)) != NULL) {
        if (!strcmp(rec->d_name, ".") || !strcmp(rec->d_name, "..")) {
            continue;
        }

        char device_path[PATH_MAX] = {0};
        snprintf(device_path, sizeof(device_path) - 1, "/dev/%s", rec->d_name);

        char device_size_file[PATH_MAX] = {0};
        snprintf(device_size_file, sizeof(device_size_file) - 1, "/sys/block/%s/size", rec->d_name);

        char device_model_file[PATH_MAX] = {0};
        snprintf(device_model_file, sizeof(device_model_file) - 1, "/sys/block/%s/device/model", rec->d_name);

        char line[255] = {0};
        FILE *fp;

        size_t device_size;
        fp = fopen(device_size_file, "r");
        if (!fp) {
            device_size = 0;
        } else {
            if (!fgets(line, sizeof(line) - 1, fp)) {
                perror("Unable to read from file");
                continue;
            }
            device_size = strtoull(line, NULL, 10);
            fclose(fp);
        }

        char device_model[255] = {0};
        fp = fopen(device_model_file, "r");
        if (!fp) {
            // no model file
            strcpy(device_model, "Unnamed");
        } else {
            if (!fgets(device_model, sizeof(device_model) - 1, fp)) {
                perror("Unable to read device model");
                continue;
            }
            fclose(fp);
        }

        rstrip(device_model);
        result[i] = calloc(1, sizeof(*result[0]));
        result[i]->model = strdup(device_model);
        result[i]->path = strdup(rec->d_name);
        result[i]->size = device_size;
        i++;
    }

    *total = devices_total;
    closedir(dp);
    return result;
}
#endif
