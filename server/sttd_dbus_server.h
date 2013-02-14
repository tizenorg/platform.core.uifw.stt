/*
* Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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



#ifndef __STTD_DBUS_SERVER_h__
#define __STTD_DBUS_SERVER_h__


#ifdef __cplusplus
extern "C" {
#endif

#include <dbus/dbus.h>


int sttd_dbus_server_hello(DBusConnection* conn, DBusMessage* msg);

/*
* Dbus Server functions for APIs
*/ 

int sttd_dbus_server_initialize(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_finalize(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_get_support_lang(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_get_default_lang(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_is_partial_result_supported(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_get_audio_volume(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_start(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_stop(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_cancel(DBusConnection* conn, DBusMessage* msg);


/*
* Dbus Server functions for Setting
*/ 

int sttd_dbus_server_setting_initialize(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_finalize(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_engine_list(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_engine(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_set_engine(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_language_list(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_default_language(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_set_default_language(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_profanity_filter(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_set_profanity_filter(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_punctuation_override(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_set_punctuation_override(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_silence_detection(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_set_silence_detection(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_get_engine_setting(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_setting_set_engine_setting(DBusConnection* conn, DBusMessage* msg);

/*
* Dbus Server functions for stt daemon internal
*/ 

int sttd_dbus_server_stop_by_daemon(DBusMessage* msg);


#ifdef __cplusplus
}
#endif


#endif	/* __STTD_DBUS_SERVER_h__ */
