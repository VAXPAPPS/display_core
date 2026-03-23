#include "services/sysinfo_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <mntent.h>

struct _DcSysinfoService {
    GObject parent_instance;
};

G_DEFINE_TYPE(DcSysinfoService, dc_sysinfo_service, G_TYPE_OBJECT)

static void dc_sysinfo_service_class_init(DcSysinfoServiceClass *klass) {
    (void)klass;
}

static void dc_sysinfo_service_init(DcSysinfoService *self) {
    (void)self;
}

DcSysinfoService *dc_sysinfo_service_new(void) {
    return g_object_new(DC_TYPE_SYSINFO_SERVICE, NULL);
}

void dc_sysinfo_service_get_os_info(DcSysinfoService *self, DcOsInfo *info) {
    (void)self;
    memset(info, 0, sizeof(DcOsInfo));

    // OS Name from /etc/os-release
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *start = line + 12;
                if (*start == '"') start++;
                char *end = strchr(start, '"');
                if (!end) end = strchr(start, '\n');
                if (end) *end = '\0';
                strncpy(info->os_name, start, 255);
                break;
            }
        }
        fclose(fp);
    }
    if (info->os_name[0] == '\0') {
        strcpy(info->os_name, "Unknown Linux");
    }

    // Desktop Environment
    char *de = getenv("XDG_CURRENT_DESKTOP");
    if (!de) de = getenv("DESKTOP_SESSION");
    if (de && strlen(de) > 0) {
        strncpy(info->desktop_env, de, 127);
    } else {
        strcpy(info->desktop_env, "VAXP-DE");
    }

    // Kernel & OS Type
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        strncpy(info->kernel_version, buffer.release, 255);
        strncpy(info->os_type, buffer.machine, 127);
    } else {
        strcpy(info->kernel_version, "Unknown");
        strcpy(info->os_type, "Unknown");
    }

    // Computer Model
    FILE *mfp = fopen("/sys/class/dmi/id/sys_vendor", "r");
    FILE *pfp = fopen("/sys/class/dmi/id/product_name", "r");
    char vendor[128] = "Unknown";
    char product[128] = "Unknown";
    
    if (mfp) {
        if (fgets(vendor, sizeof(vendor), mfp)) {
            vendor[strcspn(vendor, "\n")] = 0;
        }
        fclose(mfp);
    }
    if (pfp) {
        if (fgets(product, sizeof(product), pfp)) {
            product[strcspn(product, "\n")] = 0;
        }
        fclose(pfp);
    }
    snprintf(info->computer_model, 255, "%s %s", vendor, product);

    // Shell
    char *shell = getenv("SHELL");
    if (shell) {
        char *last_slash = strrchr(shell, '/');
        if (last_slash) {
            strncpy(info->shell_type, last_slash + 1, 127);
        } else {
            strncpy(info->shell_type, shell, 127);
        }
    } else {
        strcpy(info->shell_type, "Unknown");
    }
}

void dc_sysinfo_service_get_mem_info(DcSysinfoService *self, DcMemInfo *info) {
    (void)self;
    memset(info, 0, sizeof(DcMemInfo));
    
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;

    char line[256];
    long mem_total = 0, mem_free = 0, mem_available = 0, buffers = 0, cached = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemFree: %ld kB", &mem_free) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &mem_available) == 1) continue;
        if (sscanf(line, "Buffers: %ld kB", &buffers) == 1) continue;
        if (sscanf(line, "Cached: %ld kB", &cached) == 1) continue;
    }
    fclose(fp);

    double kb_to_gb = 1024.0 * 1024.0;
    info->total_gb = mem_total / kb_to_gb;
    info->free_gb = mem_free / kb_to_gb;
    
    if (mem_available > 0) {
        double avail = mem_available / kb_to_gb;
        info->used_gb = info->total_gb - avail;
    } else {
        long used = mem_total - mem_free - buffers - cached;
        info->used_gb = used / kb_to_gb;
    }
}

void dc_sysinfo_service_get_storage_info(DcSysinfoService *self, DcStorageInfo *info) {
    (void)self;
    memset(info, 0, sizeof(DcStorageInfo));
    
    FILE *fp = setmntent("/etc/mtab", "r");
    if (!fp) return;

    struct mntent *ent;
    int count = 0;
    while ((ent = getmntent(fp)) != NULL && count < 64) {
        if (strncmp(ent->mnt_fsname, "/dev/", 5) == 0) {
            struct statvfs stat;
            if (statvfs(ent->mnt_dir, &stat) == 0) {
                double block_size = stat.f_frsize;
                double total = (stat.f_blocks * block_size) / (1024.0 * 1024.0 * 1024.0);
                double free = (stat.f_bfree * block_size) / (1024.0 * 1024.0 * 1024.0);
                double used = total - free;

                if (total > 0) {
                    info->total_gb += total;
                    info->free_gb += free;
                    info->used_gb += used;
                }
            }
        }
        count++;
    }
    endmntent(fp);
}

DcGpuInfo *dc_sysinfo_service_get_gpu_info(DcSysinfoService *self, int *out_count) {
    (void)self;
    *out_count = 0;

    FILE *fp = popen("lspci -v 2>/dev/null", "r");
    if (!fp) return NULL;

    DcGpuInfo *gpus = g_new0(DcGpuInfo, 10);
    int count = 0;
    char line[512];
    int current_gpu = -1;
    long current_vram_total = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] != '\t' && line[0] != ' ') {
            if (current_gpu >= 0 && count < 10) {
                gpus[count].vram_mb = current_vram_total;
                count++;
                current_gpu = -1;
                current_vram_total = 0;
            }

            if (strstr(line, "VGA compatible controller") || strstr(line, "3D controller") || strstr(line, "Display controller")) {
                current_gpu = count;
                current_vram_total = 0;
                
                char *colon = strchr(line, ':');
                if (colon) {
                    colon = strchr(colon + 1, ':');
                    if (colon) {
                        colon++;
                        while (*colon == ' ') colon++;
                        colon[strcspn(colon, "\n")] = 0;
                        
                        char *rev = strstr(colon, " (rev");
                        if (rev) *rev = '\0';
                        
                        strncpy(gpus[current_gpu].name, colon, 255);
                    }
                }
            }
        } else if (current_gpu >= 0) {
            char *size_ptr = strstr(line, "[size=");
            if (size_ptr && strstr(line, "prefetchable")) {
                size_ptr += 6;
                long amount = 0;
                char unit = 0;
                if (sscanf(size_ptr, "%ld%c", &amount, &unit) == 2) {
                    if (unit == 'M') current_vram_total += amount;
                    else if (unit == 'G') current_vram_total += amount * 1024;
                    else if (unit == 'K') current_vram_total += amount / 1024;
                }
            }
        }
    }
    
    if (current_gpu >= 0 && count < 10) {
        gpus[count].vram_mb = current_vram_total;
        count++;
    }

    pclose(fp);
    
    *out_count = count;
    if (count == 0) {
        g_free(gpus);
        return NULL;
    }
    return gpus;
}
