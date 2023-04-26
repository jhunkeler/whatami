#ifndef WHATAMI_COMMON_H
#define WHATAMI_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <cpuid.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>

#if defined(__x86_64__) || defined(__i386__)
#include "x86.h"
#endif

#if defined(__linux__)
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

union regs_t {
    struct {
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;
    }  gpr;
    unsigned int bytes[15];
};

struct Block_Device {
    char *path;
    char *model;
    size_t size;
};

unsigned int CPUID(unsigned int leaf, union regs_t *reg);
int is_cpu_hyperthreaded();
int is_cpu_virtual();
char *get_sys_product();
#if defined(__APPLE__)
char *get_sys_product_darwin();
#endif
unsigned int get_cpu_count();
char *get_cpu_manufacturer();
char *get_cpu_vendor();


size_t rstrip(char *s);
int get_sys_os_dist(char **name, char **version);
ssize_t get_sys_memory();
char *get_sys_dmi_product();
struct Block_Device **get_block_devices(size_t *total);

#endif //WHATAMI_COMMON_H
