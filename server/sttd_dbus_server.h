/*
* Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved 
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

int sttd_dbus_server_get_support_engines(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_set_current_engine(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_get_current_engine(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_check_app_agreed(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_get_support_lang(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_get_default_lang(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_is_recognition_type_supported(DBusConnection* conn, DBusMessage* msg);


int sttd_dbus_server_set_start_sound(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_unset_start_sound(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_set_stop_sound(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_unset_stop_sound(DBusConnection* conn, DBusMessage* msg);


int sttd_dbus_server_start(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_stop(DBusConnection* conn, DBusMessage* msg);

int sttd_dbus_server_cancel(DBusConnection* conn, DBusMessage* msg);



#ifdef __cplusplus
}
#endif


#endif	/* __STTD_DBUS_SERVER_h__ */
