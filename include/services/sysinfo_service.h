#ifndef DC_SYSINFO_SERVICE_H
#define DC_SYSINFO_SERVICE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DC_TYPE_SYSINFO_SERVICE (dc_sysinfo_service_get_type())
G_DECLARE_FINAL_TYPE(DcSysinfoService, dc_sysinfo_service, DC, SYSINFO_SERVICE, GObject)

typedef struct {
    char os_name[256];
    char desktop_env[128];
    char kernel_version[256];
    char os_type[128];
    char computer_model[256];
    char shell_type[128];
} DcOsInfo;

typedef struct {
    double total_gb;
    double used_gb;
    double free_gb;
} DcMemInfo;

typedef struct {
    double total_gb;
    double used_gb;
    double free_gb;
} DcStorageInfo;

typedef struct {
    char name[256];
    long vram_mb;
} DcGpuInfo;

DcSysinfoService *dc_sysinfo_service_new(void);

void dc_sysinfo_service_get_os_info(DcSysinfoService *self, DcOsInfo *info);
void dc_sysinfo_service_get_mem_info(DcSysinfoService *self, DcMemInfo *info);
void dc_sysinfo_service_get_storage_info(DcSysinfoService *self, DcStorageInfo *info);

/* Returns a newly allocated array of DcGpuInfo. Sets *count. Caller must g_free() the array. */
DcGpuInfo *dc_sysinfo_service_get_gpu_info(DcSysinfoService *self, int *count);

G_END_DECLS

#endif /* DC_SYSINFO_SERVICE_H */
