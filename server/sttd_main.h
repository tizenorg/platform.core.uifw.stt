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


#ifndef __STTD_MAIN_H_
#define __STTD_MAIN_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <dlog.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
* STT Daemon Define
*/

#define TAG_STTD "sttd"

#define ENGINE_DIRECTORY_DEFAULT		"/usr/lib/voice/stt/1.0/engine"
#define ENGINE_DIRECTORY_DEFAULT_SETTING	"/usr/lib/voice/stt/1.0/setting"

#define ENGINE_DIRECTORY_DOWNLOAD		"/opt/apps/voice/stt/1.0/engine"
#define ENGINE_DIRECTORY_DOWNLOAD_SETTING	"/opt/apps/voice/stt/1.0/setting"

/* for debug message */
#define RECORDER_DEBUG

typedef enum {
	STTD_ERROR_NONE			= 0,		/**< Successful */
	STTD_ERROR_OUT_OF_MEMORY	= -ENOMEM,	/**< Out of Memory */
	STTD_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	STTD_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	STTD_ERROR_TIMED_OUT		= -ETIMEDOUT,	/**< No answer from the daemon */
	STTD_ERROR_RECORDER_BUSY	= -EBUSY,	/**< Busy recorder */
	STTD_ERROR_OUT_OF_NETWORK	= -ENETDOWN,	/**< Out of network */
	STTD_ERROR_INVALID_STATE	= -0x0100031,	/**< Invalid state */
	STTD_ERROR_INVALID_LANGUAGE	= -0x0100032,	/**< Invalid language */
	STTD_ERROR_ENGINE_NOT_FOUND	= -0x0100033,	/**< No available engine  */	
	STTD_ERROR_OPERATION_FAILED	= -0x0100034,	/**< Operation failed  */
	STTD_ERROR_NOT_SUPPORTED_FEATURE= -0x0100035	/**< Not supported feature of current engine */
}stt_error_e;

typedef struct {
	char* engine_id;
	char* engine_name;
	char* ug_name;
}engine_s;

typedef struct {
	char* key;
	char* value;
}engine_setting_s;

#ifdef __cplusplus
}
#endif

#endif	/* __STTD_MAIN_H_ */
