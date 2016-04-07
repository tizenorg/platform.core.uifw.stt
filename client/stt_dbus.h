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

 
#ifndef __STT_DBUS_H_
#define __STT_DBUS_H_

#include "stt.h"
#include "stt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBSCL_EXPORT_API
#define LIBSCL_EXPORT_API
#endif // LIBSCL_EXPORT_API


LIBSCL_EXPORT_API int stt_dbus_open_connection();

LIBSCL_EXPORT_API int stt_dbus_close_connection();


LIBSCL_EXPORT_API int stt_dbus_request_hello();

LIBSCL_EXPORT_API int stt_dbus_request_initialize(int uid, bool* silence_supported);

LIBSCL_EXPORT_API int stt_dbus_request_finalize(int uid);

LIBSCL_EXPORT_API int stt_dbus_request_set_current_engine(int uid, const char* engine_id, bool* silence_supported);

LIBSCL_EXPORT_API int stt_dbus_request_check_app_agreed(int uid, const char* appid, bool* value);

LIBSCL_EXPORT_API int stt_dbus_request_get_support_langs(int uid, stt_h stt, stt_supported_language_cb callback, void* user_data);

LIBSCL_EXPORT_API int stt_dbus_request_get_default_lang(int uid, char** language);

LIBSCL_EXPORT_API int stt_dbus_request_is_recognition_type_supported(int uid, const char* type, bool* support);

LIBSCL_EXPORT_API int stt_dbus_request_set_start_sound(int uid, const char* file);

LIBSCL_EXPORT_API int stt_dbus_request_unset_start_sound(int uid);

LIBSCL_EXPORT_API int stt_dbus_request_set_stop_sound(int uid, const char* file);

LIBSCL_EXPORT_API int stt_dbus_request_unset_stop_sound(int uid);


LIBSCL_EXPORT_API int stt_dbus_request_start(int uid, const char* lang, const char* type, int silence, const char* appid);

LIBSCL_EXPORT_API int stt_dbus_request_stop(int uid);

LIBSCL_EXPORT_API int stt_dbus_request_cancel(int uid);


#ifdef __cplusplus
}
#endif

#endif /* __STT_DBUS_H_ */
