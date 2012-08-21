/*
*  Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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


#ifndef __STTD_CLIENT_DATA_H_
#define __STTD_CLIENT_DATA_H_

#include <glib.h>
#include <Ecore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	APP_STATE_CREATED	= 0,
	APP_STATE_READY		= 1,
	APP_STATE_RECORDING	= 2,
	APP_STATE_PROCESSING	= 3
}app_state_e;

typedef struct {
	int	pid;
	int	uid;
	app_state_e	state;
	Ecore_Timer*	timer;
} client_info_s;

typedef struct {
	int pid;
} setting_client_info_s;

int sttd_client_add(const int pid, const int uid);

int sttd_client_delete(const int uid);

int sttd_client_get_state(const int uid, app_state_e* state);

int sttd_client_set_state(const int uid, const app_state_e state);

int sttd_client_get_ref_count();

int sttd_client_get_pid(const int uid);

int sttd_client_get_current_recording();

int sttd_client_get_current_thinking();

int sttd_cliet_set_timer(int uid, Ecore_Timer* timer);

int sttd_cliet_get_timer(int uid, Ecore_Timer** timer);

int sttd_client_get_list(int** uids, int* uid_count);


int sttd_setting_client_add(int pid);

int sttd_setting_client_delete(int pid);

bool sttd_setting_client_is(int pid);

#ifdef __cplusplus
}
#endif

#endif	/* __STTD_CLIENT_DATA_H_ */

