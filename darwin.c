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
    struct Block_Device **result;
    size_t devices_total;
    size_t count;
    struct statfs *mnt;
    int mnt_flags;

    if (!(devices_total = getmntinfo(&mnt, mnt_flags))) {
        perror("Unable to read mount points");
        return NULL;
    }

    result = calloc(devices_total + 1, sizeof(*result));
    if (!result) {
        perror("Unable to allocate device array");
        return NULL;
    }

    count = 0;
    char path_prev[PATH_MAX] = {0};
    for (size_t i = 0; i < devices_total; i++) {
        if (strncmp(mnt[i].f_mntfromname, "/dev/", 5) != 0) {
            continue;
        }
        result[count] = calloc(1, sizeof(*result[0]));
        if (!result[count]) {
            perror("Unable to allocate mount point record");
            exit(1);
        }
        char path_temp[PATH_MAX] = {0};
        size_t dnum = 0;
        sscanf(&mnt[i].f_mntfromname[5], "disk%zus%*s", &dnum);
        sprintf(path_temp, "disk%zu", dnum);
        if (strlen(path_prev) && !strcmp(path_prev, path_temp)) {
            continue;
        }
        strcpy(path_prev, path_temp);
        result[count]->path = strdup(path_temp);
        result[count]->model = strdup("Unnamed");
        result[count]->size = (mnt[i].f_bsize * mnt[i].f_blocks) / 1024;
        count++;
    }
    if (devices_total != count) {
        *total = count;
    } else {
        *total = devices_total;
    }
    return result;
}
#endif
