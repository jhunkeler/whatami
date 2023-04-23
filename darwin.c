#if defined(__APPLE__)
#include "common.h"

char *get_sys_product() {
    char model[100] = {0};
    size_t len;

    len = sizeof(model);
    sysctlbyname("hw.model", model, &len, NULL, 0);
    return strdup(model);
}

size_t get_sys_memory() {
    size_t mem_size;
    size_t len;

    mem_size= 0;
    len = sizeof(mem_size);
    sysctlbyname("hw.memsize", &mem_size, &len, NULL, 0);

    return mem_size / 1024;
}

int *get_sys_os_dist(char **a, char **b) {
    char version[255] = {0};
    size_t version_len;

    version_len = sizeof(version);
    sysctlbyname("kern.osproductversion", version, &version_len, NULL, 0);
    *a = strdup("MacOS");
    *b = strdup(version);
    return 0;
}

void *get_block_devices(void *x) {
    return NULL;
}
#endif