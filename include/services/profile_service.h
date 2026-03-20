#ifndef DC_PROFILE_SERVICE_H
#define DC_PROFILE_SERVICE_H

#include "domain/display_types.h"

gboolean dc_profile_service_save(const char *profile_name, GPtrArray *configs, char **error_message);
gboolean dc_profile_service_load(const char *profile_name, GPtrArray **configs, char **error_message);
gboolean dc_profile_service_list(GStrv *profile_names, char **error_message);

#endif
