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
int sttd_initialize();

int sttd_finalize();

Eina_Bool sttd_cleanup_client(void *data);

Eina_Bool sttd_get_daemon_exist();

/*
* API for client
*/

int sttd_server_initialize(int pid, int uid, bool* silence);

int sttd_server_finalize(int uid);

int sttd_server_get_supported_engines(int uid, GSList** engine_list);

int sttd_server_set_current_engine(int uid, const char* engine_id, bool* silence);

int sttd_server_get_current_engine(int uid, char** engine_id);

int sttd_server_check_agg_agreed(int uid, const char* appid, bool* available);

int sttd_server_get_supported_languages(int uid, GSList** lang_list);

int sttd_server_get_current_langauage(int uid, char** current_lang);

int sttd_server_set_engine_data(int uid, const char* key, const char* value);

int sttd_server_is_recognition_type_supported(int uid, const char* type, int* support);

int sttd_server_set_start_sound(int uid, const char* file);

int sttd_server_set_stop_sound(int uid, const char* file);


int sttd_server_get_audio_volume(int uid, float* current_volume);

int sttd_server_start(int uid, const char* lang, const char* recognition_type, int silence, const char* appid);

int sttd_server_stop(int uid);

int sttd_server_cancel(int uid);



#ifdef __cplusplus
}
#endif

#endif /* __STTD_SERVER_H_ */
