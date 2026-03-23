#include "app_controller_internal.h"
#include <string.h>

/* ----------------------------------------------------- */
/*  Public Setup                                        */
/* ----------------------------------------------------- */

void dc_app_about_load(DcAppController *app) {
    if (!app->sysinfo_service || !app->about_page) return;

    // 1. OS Info
    DcOsInfo os;
    dc_sysinfo_service_get_os_info(app->sysinfo_service, &os);
    dc_about_page_set_os_info(app->about_page, os.os_name, os.desktop_env, os.kernel_version, os.os_type, os.computer_model, os.shell_type);

    // 2. Hardware Info (RAM, Disk)
    DcMemInfo mem;
    dc_sysinfo_service_get_mem_info(app->sysinfo_service, &mem);
    
    DcStorageInfo disk;
    dc_sysinfo_service_get_storage_info(app->sysinfo_service, &disk);

    char ram_str[128];
    snprintf(ram_str, sizeof(ram_str), "%.2f GB", mem.total_gb);
    
    char disk_str[128];
    snprintf(disk_str, sizeof(disk_str), "%.2f GB", disk.total_gb);

    dc_about_page_set_hardware_info(app->about_page, ram_str, disk_str);

    // 3. Graphics Adapters
    int gpu_count = 0;
    DcGpuInfo *gpus = dc_sysinfo_service_get_gpu_info(app->sysinfo_service, &gpu_count);
    
    if (gpu_count > 0 && gpus != NULL) {
        if (gpu_count == 1) {
            dc_about_page_set_graphics_info(app->about_page, gpus[0].name);
        } else {
            char gpus_str[1024] = "";
            for (int i = 0; i < gpu_count; i++) {
                strncat(gpus_str, gpus[i].name, sizeof(gpus_str) - strlen(gpus_str) - 1);
                if (i < gpu_count - 1) {
                    strncat(gpus_str, "\n", sizeof(gpus_str) - strlen(gpus_str) - 1);
                }
            }
            dc_about_page_set_graphics_info(app->about_page, gpus_str);
        }
        g_free(gpus);
    } else {
        dc_about_page_set_graphics_info(app->about_page, "No Graphics Adapter Detected");
    }
}
