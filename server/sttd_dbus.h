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


#ifndef __STTD_DBUS_h__
#define __STTD_DBUS_h__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBSCL_EXPORT_API
#define LIBSCL_EXPORT_API
#endif // LIBSCL_EXPORT_API


LIBSCL_EXPORT_API int sttd_dbus_open_connection();

LIBSCL_EXPORT_API int sttd_dbus_close_connection();


LIBSCL_EXPORT_API int sttdc_send_hello(int uid);

LIBSCL_EXPORT_API int sttdc_send_set_volume(int uid, float volume);

LIBSCL_EXPORT_API int sttdc_send_set_state(int uid, int state);

LIBSCL_EXPORT_API int sttdc_send_result(int uid, int event, const char** data, int data_count, const char* result_msg);

LIBSCL_EXPORT_API int sttdc_send_error_signal(int uid, int reason, const char *err_msg);


#ifdef __cplusplus
}
#endif

#endif	/* __STTD_DBUS_h__ */
