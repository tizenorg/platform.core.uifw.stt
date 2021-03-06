/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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


#ifndef __STTD_DBUS_h__
#define __STTD_DBUS_h__

#ifdef __cplusplus
extern "C" {
#endif

int sttd_dbus_open_connection();

int sttd_dbus_close_connection();


int sttdc_send_hello(int uid);

int sttdc_send_get_state(int uid, int* state);

int sttdc_send_result(int uid, const char* type, const char** data, int data_count, const char* result_msg);

int sttdc_send_partial_result(int uid, const char* data);

int sttdc_send_error_signal(int uid, int reason, char *err_msg);

int sttdc_send_set_state(int uid, int state);

int sttd_send_stop_recognition_by_daemon(int uid);

#ifdef __cplusplus
}
#endif

#endif	/* __STTD_DBUS_h__ */
