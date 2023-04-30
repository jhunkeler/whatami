#if defined(_MSC_VER) || defined(__MINGW32__)
#include "common.h"
#include <VersionHelpers.h>

static OSVERSIONINFO get_os_version_info() {
    static OSVERSIONINFO osinfo;
    static int cached = 0;
    if (cached) {
        return osinfo;
    }
    memset(&osinfo, 0, sizeof(OSVERSIONINFOA));
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA((OSVERSIONINFOA *) &osinfo);
    cached = 1;
    return osinfo;
}

int uname(struct utsname *buf) {
    char value[255] = {0};
    unsigned long size;

    memset(buf, 0, sizeof(struct utsname));
    size = sizeof(value) - 1;
    strcpy_s(buf->sysname, sizeof(buf->sysname) - 1, "Win" BITSUFFIX);
    GetComputerNameEx(ComputerNameDnsFullyQualified, value, &size);
    strcpy_s(buf->nodename, sizeof(buf->nodename) - 1, value);

    SYSTEM_INFO sysinfo;
    GetNativeSystemInfo(&sysinfo);
    switch (sysinfo.wProcessorArchitecture) {
        case PROCESSOR_AMD_X8664:
        case PROCESSOR_ARCHITECTURE_AMD64:
            strcpy_s(buf->machine, sizeof(buf->machine) - 1, "x86_64");
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            strcpy_s(buf->machine, sizeof(buf->machine) - 1, "arm64");
            break;
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            strcpy_s(buf->machine, sizeof(buf->machine) - 1, "unknown");
            break;
    }

    OSVERSIONINFO osinfo;
    memset(&osinfo, 0, sizeof(OSVERSIONINFOA));
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA((OSVERSIONINFOA *) &osinfo);
    sprintf_s(buf->release, sizeof(buf->release) - 1, "%lu.%lu", osinfo.dwMajorVersion, osinfo.dwMinorVersion);
    sprintf_s(buf->version, sizeof(buf->release) - 1, "build %lu", osinfo.dwBuildNumber);
    return 0;
}

int get_sys_os_dist(char **name, char **version) {
    char lversion[255];
    OSVERSIONINFO osinfo;
    osinfo = get_os_version_info();

    if (IsWindows10OrGreater()) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows 10");
    } else if (IsWindows8OrGreater()) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows 8");
    } else if (IsWindows7OrGreater()) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows 7");
    } else if (IsWindowsVistaOrGreater()) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows Vista");
    } else if (IsWindowsXPOrGreater()) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows XP");
    } else if (IsWindowsServer()) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows Server");
    } else if (osinfo.dwMajorVersion == 5 && osinfo.dwMinorVersion == 0) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows 2000");
    } else if (osinfo.dwMajorVersion == 4 && osinfo.dwMinorVersion == 0 && osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows NT 4.0");
    } else if (osinfo.dwMajorVersion == 4 && osinfo.dwMinorVersion == 90 && osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows ME");
    } else if (osinfo.dwMajorVersion == 4 && osinfo.dwMinorVersion == 10 && osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows 98");
    } else {
        strcpy_s(lversion, sizeof(lversion) - 1, "Windows");
    }

    *name = _strdup("Microsoft");
    *version = _strdup(lversion);
    return 0;
}

ssize_t get_sys_memory() {
    MEMORYSTATUSEX buf;
    buf.dwLength = sizeof(buf);
    GlobalMemoryStatusEx(&buf);
    return (ssize_t) buf.ullTotalPhys / 1024;
}

struct Block_Device **get_block_devices(size_t *total) {
    struct Block_Device **device;
    char buf[256] = {0};
    DWORD len;

    // Each drive name record is 4 bytes (e.g. "C:\\0")
    len = GetLogicalDriveStringsA(sizeof(buf) - 1, buf);
    if (!len) {
        return NULL;
    }

    size_t device_count;
    device_count = len / 4;
    device = calloc(device_count + 1, sizeof(**device));
    for (DWORD i = 0, j = 0; strlen(&buf[j]) != 0 && i < device_count; i++) {
        device[i] = calloc(1, sizeof(*device[0]));
        device[i]->model = _strdup("Unnamed");
        device[i]->path = _strdup(&buf[j]);
        GetDiskFreeSpaceExA(device[i]->path, NULL, (PULARGE_INTEGER) &device[i]->size, NULL);
        device[i]->size /= 1024;
        j += 4;
    }
    *total = device_count;
    return device;
}
#endif // _WINDOWS