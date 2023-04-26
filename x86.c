#include "common.h"

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

int is_cpu_hyperthreaded() {
    union regs_t reg;

    CPUID(1, &reg);
    // Hyperthreading feature flag is located in bit 28 of EDX (regs[3])
    if (reg.gpr.edx & bit_HTT) {
        // hyperthreaded
        return 1;
    }
    return 0;
}

int is_cpu_virtual() {
    union regs_t reg;

    CPUID(1, &reg);
    // Virtualization flag is located in bit 31 of ECX
    if (reg.gpr.ecx & bit_VRT) {
        return 1;
    }
    return 0;
}

char *get_sys_product() {
    union regs_t reg;
    static char vendor[255] = {0};

    if (is_cpu_virtual()) {
        CPUID(0x40000000, &reg);
        strncat(vendor, (char *) &reg.bytes[1], sizeof(reg.bytes));
        rstrip(vendor);
    }
#if defined(__linux__)
    if (!strlen(vendor)) {
        strcpy(vendor, get_sys_dmi_product());
        rstrip(vendor);
    }
#elif defined(__APPLE__)
    if (!strlen(vendor)) {
        strcpy(vendor, get_sys_product_darwin());
        rstrip(vendor);
    }
#endif
    return vendor;
}

unsigned int get_cpu_count() {
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

char *get_cpu_manufacturer() {
    union regs_t reg;
    static char manufacturer[255] = {0};

    CPUID(0, &reg);
    strncat(manufacturer, (char *) &reg.bytes[1], 4);
    strncat(manufacturer, (char *) &reg.bytes[3], 4);
    strncat(manufacturer, (char *) &reg.bytes[2], 4);
    return manufacturer;
}

char *get_cpu_vendor() {
    union regs_t reg;
    static char vendor[255] = {0};

    for (unsigned int leaf = 2; leaf < 5; leaf++) {
        CPUID(0x80000000 + leaf, &reg);
        strncat(vendor, (char *) reg.bytes, sizeof(reg.bytes));
    }

    rstrip(vendor);
    return vendor;
}
