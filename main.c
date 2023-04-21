#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <cpuid.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>

#if defined(__linux__)
#define get_sys_memory get_memory_linux
#define get_sys_os_dist get_os_dist_linux
#else
#error No platform drivers
#endif

#if defined(__x86_64__) || defined(__i386__)
// Hyperthreading
#define bit_HTT (1 << 28)
// Virtualization
#define bit_VRT (1 << 31)

#define is_cpu_hyperthreaded is_cpu_hyperthreaded_x86
#define is_cpu_virtual is_cpu_virtual_x86
#define get_cpu_vendor get_cpu_vendor_x86
#define get_cpu_manufacturer get_cpu_manufacturer_x86
#define get_cpu_count get_cpu_count_x86
#define get_sys_product get_sys_product_x86
#else
#error No driver to retrieve CPU information
#endif

union regs_t {
    struct {
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;
    }  gpr;
    unsigned int bytes[16];
};

struct Block_Device {
    char path[PATH_MAX];
    char model[255];
    size_t size;
};

/***
 * Strip whitespace from end of string
 * @param s string
 * @return count of characters stripped
 */
size_t rstrip(char *s) {
    char *ch;
    size_t i;

    i = 0;
    ch = &s[strlen(s)];
    if (ch) {
        while (isspace(*ch) || iscntrl(*ch)) {
            *ch = '\0';
            --ch;
            i++;
        }
    }
    return i;
}

/***
 *
 * @param leaf
 * @param reg union regs_t
 * @return contents of eax register
 */
unsigned int CPUID(unsigned int leaf, union regs_t *reg) {
    memset(reg, 0, sizeof(*reg));
    __get_cpuid(leaf, &reg->gpr.eax, &reg->gpr.ebx, &reg->gpr.ecx, &reg->gpr.edx);
    return reg->gpr.eax;
}

int get_os_dist_linux(char **name, char **version) {
    char buf[255] = {0};
    const char *filename = "/etc/os-release";
    FILE *fp;

    if (access(filename, R_OK) < 0) {
        return -1;
    }

    fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
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
            *name = strdup(value);
        }
        if (!strcmp(key, "VERSION") || !strcmp(key, "VERSION_ID") || !strcmp(key, "BUILD_ID")) {
            *version = strdup(value);
        }
    }
    fclose(fp);
    return 0;
}

ssize_t get_memory_linux() {
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

int is_cpu_hyperthreaded_x86() {
    union regs_t reg;

    CPUID(1, &reg);
    // Hyperthreading feature flag is located in bit 28 of EDX (regs[3])
    if (reg.gpr.edx & bit_HTT) {
        // hyperthreaded
        return 1;
    }
    return 0;
}

int is_cpu_virtual_x86() {
    union regs_t reg;

    CPUID(1, &reg);
    // Virtualization flag is located in bit 31 of ECX
    if (reg.gpr.ecx & bit_VRT) {
        return 1;
    }
    return 0;
}

char *get_sys_dmi_product_linux() {
    FILE *fp;
    char *buf;
    const int buf_size = 255;

    buf = calloc(buf_size, sizeof(*buf));
    if (!buf) {
        return NULL;
    }

    fp = fopen("/sys/class/dmi/id/product_name", "r");
    if (!fp) {
        free(buf);
        return NULL;
    }

    if (!fgets(buf, buf_size, fp)) {
        perror("Unable to read system vendor");
        if (fp != NULL) {
            free(buf);
            fclose(fp);
        }
        return NULL;
    }

    fclose(fp);
    return buf;
}

char *get_sys_product_x86() {
    union regs_t reg;
    char *vendor;

    vendor = NULL;
    if (is_cpu_virtual_x86()) {
        vendor = calloc(255, sizeof(*vendor));
        if (!vendor) {
            return NULL;
        }
        CPUID(0x40000000, &reg);
        strncat(vendor, (char *) &reg.bytes[1], sizeof(reg.bytes));
        rstrip(vendor);
    }

#if defined(__linux__)
    if (!vendor || !strlen(vendor)) {
        vendor = get_sys_dmi_product_linux();
        rstrip(vendor);
    }
#endif

    return vendor;
}

unsigned int get_cpu_count_x86() {
    union regs_t reg;
    unsigned int result;

    if (is_cpu_hyperthreaded()) {
        CPUID(1, &reg);
        // cpu count is located in bits 16:23 of EBX
        result = reg.gpr.ebx >> 16 & 0xff;
    } else { // Legacy check
        // Core Count is located in 0:7 of ECX
        CPUID(0x80000008, &reg);
        result = 1 + (reg.gpr.ecx & 0xff);
    }

#if defined(__linux__) || (defined(__APPLE__) || defined(TARGET_OS_MAC))
    if (result == 1) {
        // One CPU might indicate we were unable to poll the information
        // See what the kernel thinks
        result = sysconf(_SC_NPROCESSORS_ONLN);
    }
#endif
    return result;
}

char *get_cpu_manufacturer_x86() {
    union regs_t reg;
    char *manufacturer;

    CPUID(0, &reg);
    manufacturer = calloc(sizeof(reg.bytes), sizeof(*reg.bytes));
    if (!manufacturer) {
        return NULL;
    }
    strncat(manufacturer, (char *) &reg.bytes[1], 4);
    strncat(manufacturer, (char *) &reg.bytes[3], 4);
    strncat(manufacturer, (char *) &reg.bytes[2], 4);
    return manufacturer;
}

char *get_cpu_vendor_x86() {
    union regs_t reg;
    char *vendor;

    vendor = calloc(sizeof(reg.bytes) * 3, sizeof(*reg.bytes));
    for (unsigned int leaf = 2; leaf < 5; leaf++) {
        CPUID(0x80000000 + leaf, &reg);
        strncat(vendor, (char *) reg.bytes, sizeof(reg.bytes));
    }

    rstrip(vendor);
    return vendor;
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

    result = calloc(devices_total + 1, sizeof(result));
    for (size_t d = 0; d < devices_total; d++) {
        result[d] = calloc(1, sizeof(*result[0]));
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
        fp = fopen(device_size_file, "r");
        if (!fp) {
            perror(device_size_file);
            continue;
        }
        if (!fgets(line, sizeof(line) - 1, fp)) {
            perror("Unable to read from file");
            continue;
        }
        fclose(fp);

        size_t device_size;
        device_size = strtoull(line, NULL, 10);

        char device_model[255] = {0};
        if (access(device_model_file, R_OK) < 0) {
            perror(device_model_file);
            continue;
        }

        fp = fopen(device_model_file, "r");
        if (!fp) {
            perror(device_model_file);
            continue;
        }
        if (!fgets(device_model, sizeof(line) - 1, fp)) {
            perror("Unable to read device model");
            continue;
        }
        fclose(fp);

        rstrip(device_model);
        if (strlen(device_model)) {
            strcpy(result[i]->model, device_model);
        } else {
            strcpy(result[i]->model, "Unnamed");
        }
        strncpy(result[i]->path, rec->d_name, sizeof(result[i]->path) - 1);
        result[i]->size = device_size;
        i++;
    }

    *total = devices_total;
    closedir(dp);
    return result;
}

int cmp_block_device(const void *aa, const void *bb) {
    const char *a = ((struct Block_Device *) aa)->path;
    const char *b = ((struct Block_Device *) bb)->path;
    return strcmp(a, b) == 0;
}

int main() {
    char *sys_product;
    char *cpu_vendor;
    char *cpu_manufacturer;
    char *distro_name;
    char *distro_version;
    unsigned int cpu_count;
    size_t device_count;
    struct utsname kinfo;
    union regs_t reg;

    if (CPUID(0, &reg) && reg.gpr.eax < 0x80000004) {
        fprintf(stderr, "CPU is not supported\n");
        exit(1);
    }

    if (uname(&kinfo) < 0) {
        perror("Unable to read uts data");
        exit(1);
    }
    get_sys_os_dist(&distro_name, &distro_version);
    cpu_manufacturer = get_cpu_manufacturer();
    cpu_vendor = get_cpu_vendor();
    cpu_count = get_cpu_count();
    sys_product = get_sys_product();

    printf("HOSTNAME: %s\n", kinfo.nodename);
    printf("TYPE: %s\n", is_cpu_virtual() ? "Virtual" : "Physical");
    printf("PRODUCT: %s\n", sys_product);
    printf("OS: %s %s\n", distro_name, distro_version);
    printf("PLATFORM: %s\n", kinfo.sysname);
    printf("ARCH: %s\n", kinfo.machine);
    printf("KERNEL: %s %s\n", kinfo.release, kinfo.version);
    printf("CPU: %s (%s)\n", cpu_vendor, cpu_manufacturer);
    printf("CPUs: %u\n", cpu_count);
    printf("Hyperthreaded: %s\n", is_cpu_hyperthreaded() ? "Yes" : "No");
    printf("RAM: %0.2lfGB\n", ((double) get_sys_memory() / 1024 / 1024));
    printf("Block devices:\n");

    struct Block_Device **block_device;
    block_device = get_block_devices(&device_count);
    if (!block_device) {
        fprintf(stderr, "Unable to enumerate block devices\n");
    } else {
        qsort(block_device, device_count, sizeof(block_device), cmp_block_device);
        for (size_t bd = 0; bd < device_count; bd++) {
            struct Block_Device *p;
            p = block_device[bd];
            printf("  %s /dev/%s (%.2lfGB)\n", p->model, p->path, (double) p->size / 1024 / 1024);
        }
    }

    return 0;
}