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


#ifndef __STT_DEFS_H__
#define __STT_DEFS_H__

#include <tzplatform_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************************
* Definition for Dbus
*******************************************************************************************/

#define STT_CLIENT_SERVICE_NAME         "org.tizen.voice.sttclient"
#define STT_CLIENT_SERVICE_OBJECT_PATH  "/org/tizen/voice/sttclient"
#define STT_CLIENT_SERVICE_INTERFACE    "org.tizen.voice.sttclient"

#define STT_SERVER_SERVICE_NAME         "org.tizen.voice.sttserver"
#define STT_SERVER_SERVICE_OBJECT_PATH  "/org/tizen/voice/sttserver"
#define STT_SERVER_SERVICE_INTERFACE    "org.tizen.voice.sttserver"


/******************************************************************************************
* Message Definition for Client
*******************************************************************************************/

#define STT_METHOD_HELLO		"stt_method_hello"
#define STT_METHOD_INITIALIZE		"stt_method_initialize"
#define STT_METHOD_FINALIZE		"stt_method_finalilze"
#define STT_METHOD_SET_CURRENT_ENGINE	"stt_method_set_current_engine"
#define STT_METHOD_GET_SUPPORT_LANGS	"stt_method_get_support_langs"
#define STT_METHOD_GET_CURRENT_LANG	"stt_method_get_current_lang"
#define STT_METHOD_IS_TYPE_SUPPORTED	"stt_method_is_recognition_type_supported"
#define STT_METHOD_CHECK_APP_AGREED	"stt_method_check_app_agreed"

#define STT_METHOD_SET_START_SOUND	"stt_method_set_start_sound"
#define STT_METHOD_UNSET_START_SOUND	"stt_method_unset_start_sound"
#define STT_METHOD_SET_STOP_SOUND	"stt_method_set_stop_sound"
#define STT_METHOD_UNSET_STOP_SOUND	"stt_method_unset_stop_sound"

#define STT_METHOD_START		"stt_method_start"
#define STT_METHOD_STOP			"stt_method_stop"
#define STT_METHOD_CANCEL		"stt_method_cancel"
#define STT_METHOD_START_FILE_RECONITION "stt_method_start_recognition"

#define STTD_METHOD_RESULT		"sttd_method_result"
#define STTD_METHOD_ERROR		"sttd_method_error"
#define STTD_METHOD_HELLO		"sttd_method_hello"
#define STTD_METHOD_SET_STATE		"sttd_method_set_state"
#define STTD_METHOD_SET_VOLUME		"sttd_method_set_volume"


/******************************************************************************************
* Defines for configuration
*******************************************************************************************/

#define STT_TIME_INFO_PATH		tzplatform_mkpath(TZ_USER_HOME, "share/.voice/stt-time.xml")

#define STT_CONFIG			tzplatform_mkpath(TZ_USER_HOME, "share/.voice/stt-config.xml")
#define STT_DEFAULT_CONFIG		tzplatform_mkpath(TZ_SYS_RO_SHARE, "/voice/stt/1.0/stt-config.xml")

#define STT_DEFAULT_ENGINE		tzplatform_mkpath(TZ_SYS_RO_SHARE, "/voice/stt/1.0/engine")
#define STT_DEFAULT_ENGINE_INFO		tzplatform_mkpath(TZ_SYS_RO_SHARE, "/voice/stt/1.0/engine-info")
#define STT_DEFAULT_ENGINE_SETTING	tzplatform_mkpath(TZ_SYS_RO_SHARE, "/voice/stt/1.0/engine-setting")

#define STT_BASE_LANGUAGE		"en_US"

#define STT_RETRY_COUNT			5

#define STT_FEATURE_PATH		"tizen.org/feature/speech.recognition"
#define STT_MIC_FEATURE_PATH		"tizen.org/feature/microphone"

#define STT_PRIVILEGE			"http://tizen.org/privilege/recorder"

#ifdef __cplusplus
}
#endif

#endif /* __STT_DEFS_H__ */
