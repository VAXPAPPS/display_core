#include "services/system_service.h"
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _DcSystemService {
    GObject parent_instance;
};

G_DEFINE_TYPE(DcSystemService, dc_system_service, G_TYPE_OBJECT)

static void dc_system_service_class_init(DcSystemServiceClass *klass) {
    (void)klass;
}

static void dc_system_service_init(DcSystemService *self) {
    (void)self;
}

DcSystemService *dc_system_service_new(void) {
    return g_object_new(DC_TYPE_SYSTEM_SERVICE, NULL);
}

/* --------------------------------------------------------
   Timezone
   -------------------------------------------------------- */
char **dc_system_service_get_timezones(DcSystemService *self, int *out_count) {
    (void)self;
    *out_count = 0;
    FILE *fp = popen("timedatectl list-timezones", "r");
    if (!fp) return NULL;

    GPtrArray *arr = g_ptr_array_new();
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            g_ptr_array_add(arr, g_strdup(line));
        }
    }
    pclose(fp);
    
    *out_count = arr->len;
    g_ptr_array_add(arr, NULL); // NULL-terminated array
    return (char **)g_ptr_array_free(arr, FALSE);
}

char *dc_system_service_get_current_timezone(DcSystemService *self) {
    (void)self;
    char *tz = NULL;
    FILE *fp = fopen("/etc/timezone", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            tz = g_strdup(line);
        }
        fclose(fp);
        return tz;
    }
    
    // Fallback: read symlink target of /etc/localtime
    char link_target[1024];
    ssize_t len = readlink("/etc/localtime", link_target, sizeof(link_target) - 1);
    if (len != -1) {
        link_target[len] = '\0';
        char *zoneinfo = strstr(link_target, "zoneinfo/");
        if (zoneinfo) {
            tz = g_strdup(zoneinfo + 9);
        }
    }
    return tz; // May be NULL
}

void dc_system_service_set_timezone(DcSystemService *self, const char *timezone) {
    (void)self;
    if (!timezone) return;
    char cmd[512];
    // Attempt pkexec if regular fails, but timedatectl triggers polkit by default inside GNOME/KDE. 
    // VAXP-OS might need pkexec depending on policy.
    snprintf(cmd, sizeof(cmd), "pkexec timedatectl set-timezone %s || timedatectl set-timezone %s", timezone, timezone);
    system(cmd);
}

/* --------------------------------------------------------
   NTP
   -------------------------------------------------------- */
gboolean dc_system_service_get_ntp_status(DcSystemService *self) {
    (void)self;
    gboolean ntp_enabled = FALSE;
    FILE *fp = popen("timedatectl status", "r");
    if (!fp) return FALSE;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "NTP service: active") || strstr(line, "systemd-timesyncd.service active: yes")) {
            ntp_enabled = TRUE;
            break;
        }
    }
    pclose(fp);
    return ntp_enabled;
}

void dc_system_service_set_ntp_status(DcSystemService *self, gboolean enabled) {
    (void)self;
    char cmd[512];
    const char *state = enabled ? "true" : "false";
    snprintf(cmd, sizeof(cmd), "pkexec timedatectl set-ntp %s || timedatectl set-ntp %s", state, state);
    system(cmd);
}

/* --------------------------------------------------------
   Locale
   -------------------------------------------------------- */
char **dc_system_service_get_locales(DcSystemService *self, int *out_count) {
    (void)self;
    *out_count = 0;
    FILE *fp = popen("locale -a", "r");
    if (!fp) return NULL;

    GPtrArray *arr = g_ptr_array_new();
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        // Filter out C, POSIX, and non-UTF8 for a clean list
        if (strlen(line) > 0 && strstr(line, "utf8") == NULL && strstr(line, "utf-8") == NULL && strstr(line, "UTF-8") == NULL) {
            // Include anyway, but standard OS prefer UTF-8. 
            // In modern Linux like Ubuntu, locale -a outputs: C, C.UTF-8, POSIX, en_US.utf8.
            // We will just let all of them in except "C" and "POSIX" for cleaner UI.
        }
        if (strcmp(line, "C") != 0 && strcmp(line, "POSIX") != 0 && strlen(line) > 0) {
            g_ptr_array_add(arr, g_strdup(line));
        }
    }
    pclose(fp);
    
    *out_count = arr->len;
    g_ptr_array_add(arr, NULL);
    return (char **)g_ptr_array_free(arr, FALSE);
}

char *dc_system_service_get_current_locale(DcSystemService *self) {
    (void)self;
    char *locale = NULL;
    FILE *fp = fopen("/etc/locale.conf", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "LANG=", 5) == 0) {
                line[strcspn(line, "\n")] = 0;
                locale = g_strdup(line + 5);
                break;
            }
        }
        fclose(fp);
        if (locale) return locale;
    }
    
    // Fallback to env
    const char *env_lang = getenv("LANG");
    if (env_lang) {
        return g_strdup(env_lang);
    }
    
    return g_strdup("C");
}

void dc_system_service_set_locale(DcSystemService *self, const char *locale) {
    (void)self;
    if (!locale) return;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pkexec localectl set-locale LANG=%s || localectl set-locale LANG=%s", locale, locale);
    system(cmd);
}
