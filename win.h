#ifndef WHATAMI_WIN_H
#define WHATAMI_WIN_H

#include <windows.h>

#if defined(_WIN32)
#undef BITSUFFIX
#define BITSUFFIX "32"
#endif

#if defined(_WIN64)
#undef BITSUFFIX
#define BITSUFFIX "64"
#endif

#if defined(_MSC_VER)
#include <intrin.h>

#if defined(__get_cpuid)
#undef __get_cpuid
#endif

static inline int __get_cpuid(int leaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    int info[4] = {0};
    __cpuid(info, leaf);
    *eax = info[0];
    *ebx = info[1];
    *ecx = info[2];
    *edx = info[3];
    return info[0];
}
#else
#include <cpuid.h>
#endif

struct utsname {
    char sysname[255];
    char nodename[255];
    char release[255];
    char version[255];
    char machine[255];
};
int uname(struct utsname *buf);

#if !defined(ssize_t)
typedef long long ssize_t;
#endif

#endif //WHATAMI_WIN_H
