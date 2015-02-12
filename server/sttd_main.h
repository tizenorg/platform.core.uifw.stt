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


#ifndef __STTD_MAIN_H_
#define __STTD_MAIN_H_

#include <dlog.h>
#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tizen.h>
#include <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
* STT Daemon Define
*/

#define TAG_STTD "sttd"

/* for debug message */
//#define CLIENT_DATA_DEBUG

typedef enum {
	STTD_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Successful */
	STTD_ERROR_OUT_OF_MEMORY	= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	STTD_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	STTD_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	STTD_ERROR_TIMED_OUT		= TIZEN_ERROR_TIMED_OUT,	/**< No answer from the daemon */
	STTD_ERROR_RECORDER_BUSY	= TIZEN_ERROR_RESOURCE_BUSY,	/**< Device or resource busy */
	STTD_ERROR_OUT_OF_NETWORK	= TIZEN_ERROR_NETWORK_DOWN,	/**< Network is down */
	STTD_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	STTD_ERROR_NOT_SUPPORTED	= TIZEN_ERROR_NOT_SUPPORTED,	/**< STT NOT supported */
	STTD_ERROR_INVALID_STATE	= TIZEN_ERROR_STT | 0x01,	/**< Invalid state */
	STTD_ERROR_INVALID_LANGUAGE	= TIZEN_ERROR_STT | 0x02,	/**< Invalid language */
	STTD_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_STT | 0x03,	/**< No available engine  */	
	STTD_ERROR_OPERATION_FAILED	= TIZEN_ERROR_STT | 0x04,	/**< Operation failed  */
	STTD_ERROR_NOT_SUPPORTED_FEATURE= TIZEN_ERROR_STT | 0x05	/**< Not supported feature of current engine */
}stt_error_e;

typedef enum {
	STTD_RESULT_STATE_DONE		= 0,			/**< Sync state change */
	STTD_RESULT_STATE_NOT_DONE	= 1			/**< Async state change */
}sttd_result_state_e;

typedef struct {
	char* engine_id;
	char* engine_name;
	char* ug_name;
}engine_s;

typedef enum {
	STTD_DAEMON_NORMAL		= 0,
	STTD_DAEMON_ON_TERMINATING	= -1
} sttd_daemon_status_e;


#ifdef __cplusplus
}
#endif

#endif	/* __STTD_MAIN_H_ */
