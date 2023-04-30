#include "common.h"

static int cmp_block_device(const void *aa, const void *bb) {
    struct Block_Device *a = *(struct Block_Device **) aa;
    struct Block_Device *b = *(struct Block_Device **) bb;

    return strcmp(a->path, b->path);
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

    if (CPUID(0, &reg) && reg.gpr.eax < 2) {
        fprintf(stderr, "CPU is not supported: 0x%08X\n", reg.gpr.eax);
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
#if defined(__x86_64__) || (__i386__)
    printf("Hyperthreaded: %s\n", is_cpu_hyperthreaded() ? "Yes" : "No");
#endif
    printf("RAM: %0.2lfGB\n", ((double) get_sys_memory() / 1024 / 1024));
    printf("Block devices:\n");

    struct Block_Device **block_device;
    device_count = 0;
    block_device = get_block_devices(&device_count);
    if (!block_device) {
        fprintf(stderr, "Unable to enumerate block devices\n");
    } else {
        qsort(block_device, device_count, sizeof(block_device[0]), cmp_block_device);
        for (size_t bd = 0; bd < device_count; bd++) {
            struct Block_Device *p;
            p = block_device[bd];
            printf("  %s %s (%.2lfGB)\n", p->model, p->path, (double) p->size / 1024 / 1024);
        }
    }

    for (size_t i = 0; i < device_count; i++) {
        free(block_device[i]->path);
        free(block_device[i]->model);
        free(block_device[i]);
    }
    free(block_device);
    free(distro_name);
    free(distro_version);

    return 0;
}
