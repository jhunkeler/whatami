#if defined(__APPLE__)
#include "common.h"

char *get_sys_product_darwin() {
    static char model[100] = {0};
    size_t len;

    len = sizeof(model);
    sysctlbyname("hw.model", model, &len, NULL, 0);
    return model;
}

ssize_t get_sys_memory() {
    size_t mem_size;
    size_t len;

    mem_size= 0;
    len = sizeof(mem_size);
    sysctlbyname("hw.memsize", &mem_size, &len, NULL, 0);

    return mem_size / 1024;
}

int get_sys_os_dist(char **name, char **version) {
    size_t version_len;
    char data[255] = {0};

    version_len = sizeof(version);
    sysctlbyname("kern.osproductversion", data, &version_len, NULL, 0);
    *name = strdup("MacOS");
    *version = strdup(data);
    return 0;
}

struct Block_Device **get_block_devices(size_t *total) {
    return NULL;
}
#endif
