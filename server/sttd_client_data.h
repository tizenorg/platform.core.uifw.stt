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
	char*	start_beep;
	char*	stop_beep;

	app_state_e	state;
/*	Ecore_Timer*	timer;	*/

	bool	app_agreed;
}client_info_s;

typedef struct {
	int	index;
	int	event;
	char*	text;
	long	start_time;
	long	end_time;
}result_time_info_s;

typedef bool (*time_callback)(int index, int event, const char* text, long start_time, long end_time, void *user_data);


int sttd_client_add(int pid, int uid);

int sttd_client_delete(int uid);

int sttd_client_get_start_sound(int uid, char** filename);

int sttd_client_set_start_sound(int uid, const char* filename);

int sttd_client_get_stop_sound(int uid, char** filename);

int sttd_client_set_stop_sound(int uid, const char* filename);

int sttd_client_get_state(int uid, app_state_e* state);

int sttd_client_set_state(int uid, app_state_e state);

int sttd_client_get_ref_count();

int sttd_client_get_pid(int uid);

#if 0
int sttd_client_get_current_recording();

int sttd_client_get_current_thinking();

int sttd_cliet_set_timer(int uid, Ecore_Timer* timer);

int sttd_cliet_get_timer(int uid, Ecore_Timer** timer);
#endif

int sttd_client_get_list(int** uids, int* uid_count);

int stt_client_set_current_recognition(int uid);

int stt_client_get_current_recognition();

int stt_client_unset_current_recognition();


int stt_client_set_app_agreed(int uid);

bool stt_client_get_app_agreed(int uid);


#ifdef __cplusplus
}
#endif

#endif	/* __STTD_CLIENT_DATA_H_ */

