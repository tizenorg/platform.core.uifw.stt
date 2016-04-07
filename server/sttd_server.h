/*
*  Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved 
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*  http://www.apache.org/licenses/LICENSE-2.0
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/


#ifndef __STTD_SERVER_H_
#define __STTD_SERVER_H_

#include <Ecore.h>
#include "sttd_main.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
* Daemon functions
*/

#ifndef LIBSCL_EXPORT_API
#define LIBSCL_EXPORT_API
#endif // LIBSCL_EXPORT_API

LIBSCL_EXPORT_API int sttd_initialize();

LIBSCL_EXPORT_API int sttd_finalize();

LIBSCL_EXPORT_API Eina_Bool sttd_cleanup_client(void *data);

LIBSCL_EXPORT_API Eina_Bool sttd_get_daemon_exist();

/*
* API for client
*/

LIBSCL_EXPORT_API int sttd_server_initialize(int pid, int uid, bool* silence);

LIBSCL_EXPORT_API int sttd_server_finalize(int uid);

LIBSCL_EXPORT_API int sttd_server_get_supported_engines(int uid, GSList** engine_list);

LIBSCL_EXPORT_API int sttd_server_set_current_engine(int uid, const char* engine_id, bool* silence);

LIBSCL_EXPORT_API int sttd_server_get_current_engine(int uid, char** engine_id);

LIBSCL_EXPORT_API int sttd_server_check_app_agreed(int uid, const char* appid, bool* available);

LIBSCL_EXPORT_API int sttd_server_get_supported_languages(int uid, GSList** lang_list);

LIBSCL_EXPORT_API int sttd_server_get_current_langauage(int uid, char** current_lang);

LIBSCL_EXPORT_API int sttd_server_set_engine_data(int uid, const char* key, const char* value);

LIBSCL_EXPORT_API int sttd_server_is_recognition_type_supported(int uid, const char* type, int* support);

LIBSCL_EXPORT_API int sttd_server_set_start_sound(int uid, const char* file);

LIBSCL_EXPORT_API int sttd_server_set_stop_sound(int uid, const char* file);


LIBSCL_EXPORT_API int sttd_server_get_audio_volume(int uid, float* current_volume);

LIBSCL_EXPORT_API int sttd_server_start(int uid, const char* lang, const char* recognition_type, int silence, const char* appid);

LIBSCL_EXPORT_API int sttd_server_stop(int uid);

LIBSCL_EXPORT_API int sttd_server_cancel(int uid);



#ifdef __cplusplus
}
#endif

#endif /* __STTD_SERVER_H_ */
