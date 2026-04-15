#ifndef DC_NETWORK_SERVICE_H
#define DC_NETWORK_SERVICE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define DC_TYPE_NETWORK_SERVICE (dc_network_service_get_type())
G_DECLARE_FINAL_TYPE(DcNetworkService, dc_network_service, DC, NETWORK_SERVICE, GObject)

/* Device types matching NM NMDeviceType */
typedef enum {
    DC_DEVICE_TYPE_UNKNOWN   = 0,
    DC_DEVICE_TYPE_ETHERNET  = 1,
    DC_DEVICE_TYPE_WIFI      = 2,
} DcNetworkDeviceType;

/* Represents a physical network device */
typedef struct _DcNetworkDevice {
    char               *object_path;
    char               *interface;      /* e.g. "wlp108s0", "eth0" */
    char               *hw_address;     /* MAC address */
    DcNetworkDeviceType type;
    guint               state;          /* NMDeviceState: 100 = activated */
    /* Wi-Fi specific */
    char               *active_ap_path; /* "/" if none */
    /* Ethernet specific */
    guint               speed;          /* Mbps */
    gboolean            carrier;        /* cable plugged in */
} DcNetworkDevice;

/* Represents a Wi-Fi Access Point */
typedef struct _DcAccessPoint {
    char    *object_path;
    char    *ssid;          /* human-readable SSID, may be NULL for hidden */
    guint8   strength;      /* 0-100 */
    gboolean secured;       /* TRUE if WEP/WPA/WPA2 */
    gboolean active;        /* TRUE if currently connected */
    gboolean has_profile;   /* TRUE if NM has saved credentials */
} DcAccessPoint;

/* IP information for a connected device */
typedef struct _DcNetworkIpInfo {
    char *address;      /* e.g. "192.168.1.5" */
    char *gateway;      /* e.g. "192.168.1.1" */
    guint prefix;       /* e.g. 24 */
} DcNetworkIpInfo;

void dc_network_device_free(DcNetworkDevice *device);
void dc_access_point_free(DcAccessPoint *ap);
void dc_network_ip_info_free(DcNetworkIpInfo *info);

/* Constructor */
DcNetworkService *dc_network_service_new(void);

/* ── Wi-Fi ─────────────────────────────────────────────── */
gboolean  dc_network_service_get_wifi_enabled(DcNetworkService *self);
void      dc_network_service_set_wifi_enabled(DcNetworkService *self, gboolean enabled);

/* Returns the first Wi-Fi device path, or NULL */
char     *dc_network_service_get_wifi_device_path(DcNetworkService *self);

/* Returns GList of DcAccessPoint*, must be freed with dc_access_point_free */
GList    *dc_network_service_get_access_points(DcNetworkService *self,
                                               const char       *wifi_device_path);

void      dc_network_service_request_scan(DcNetworkService *self,
                                          const char       *wifi_device_path);

void      dc_network_service_connect_to_ap(DcNetworkService *self,
                                           const char       *device_path,
                                           const char       *ap_path,
                                           const char       *ssid,
                                           const char       *password);

void      dc_network_service_forget_ap(DcNetworkService *self,
                                       const char       *ssid);

/* ── Ethernet ───────────────────────────────────────────── */
/* Returns GList of DcNetworkDevice* for wired devices only */
GList    *dc_network_service_get_ethernet_devices(DcNetworkService *self);

/* ── Common ─────────────────────────────────────────────── */
void      dc_network_service_disconnect(DcNetworkService *self,
                                        const char       *device_path);

/* Returns DcNetworkIpInfo* or NULL; caller frees with dc_network_ip_info_free */
DcNetworkIpInfo *dc_network_service_get_ip_info(DcNetworkService *self,
                                                 const char       *device_path);

G_END_DECLS

#endif /* DC_NETWORK_SERVICE_H */
