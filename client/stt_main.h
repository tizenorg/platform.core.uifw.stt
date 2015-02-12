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

 
#ifndef __STT_MAIN_H_
#define __STT_MAIN_H_

#include <dbus/dbus.h>
#include <dlog.h>
#include <Ecore.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "stt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_STTC "sttc" 

/** 
* @brief A structure of handle for identification
*/
struct stt_s {
	int handle;
};

typedef enum {
	STT_DAEMON_NORMAL		= 0,
	STT_DAEMON_ON_TERMINATING	= -1
} stt_daemon_status_e;

typedef enum {
	STT_RESULT_STATE_DONE		= 0,			/**< Sync state change */
	STT_RESULT_STATE_NOT_DONE	= 1			/**< Async state change */
} stt_result_state_e;

#ifdef __cplusplus
}
#endif

#endif /* __STT_CLIENT_H_ */
