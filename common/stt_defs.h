/*
*  Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************************
* Definition for Dbus
*******************************************************************************************/

#define STT_CLIENT_SERVICE_NAME         "com.samsung.voice.sttclient"
#define STT_CLIENT_SERVICE_OBJECT_PATH  "/com/samsung/voice/sttclient"
#define STT_CLIENT_SERVICE_INTERFACE    "com.samsung.voice.sttclient"

#define STT_SETTING_SERVICE_NAME        "com.samsung.voice.sttsetting"
#define STT_SETTING_SERVICE_OBJECT_PATH "/com/samsung/voice/sttsetting"
#define STT_SETTING_SERVICE_INTERFACE   "com.samsung.voice.sttsetting"

#define STT_SERVER_SERVICE_NAME         "service.connect.sttserver"
#define STT_SERVER_SERVICE_OBJECT_PATH  "/com/samsung/voice/sttserver"
#define STT_SERVER_SERVICE_INTERFACE    "com.samsung.voice.sttserver"


/******************************************************************************************
* Message Definition for Client
*******************************************************************************************/

#define STT_METHOD_HELLO		"stt_method_hello"
#define STT_METHOD_INITIALIZE		"stt_method_initialize"
#define STT_METHOD_FINALIZE		"stt_method_finalilze"
#define STT_METHOD_GET_SUPPORT_LANGS	"stt_method_get_support_langs"
#define STT_METHOD_GET_CURRENT_LANG	"stt_method_get_current_lang"
#define STT_METHOD_IS_PARTIAL_SUPPORTED	"stt_method_is_partial_result_supported"

#define STT_METHOD_START		"stt_method_start"
#define STT_METHOD_STOP			"stt_method_stop"
#define STT_METHOD_CANCEL		"stt_method_cancel"
#define STT_METHOD_START_FILE_RECONITION "stt_method_start_recognition"

#define STTD_METHOD_RESULT		"sttd_method_result"
#define STTD_METHOD_PARTIAL_RESULT	"sttd_method_partial_result"
#define STTD_METHOD_ERROR		"sttd_method_error"
#define STTD_METHOD_HELLO		"sttd_method_hello"
#define STTD_METHOD_SET_STATE		"sttd_method_set_state"
#define STTD_METHOD_GET_STATE		"sttd_method_get_state"

/******************************************************************************************
* Message Definition for Setting
*******************************************************************************************/

#define STT_SETTING_METHOD_HELLO		"stt_setting_method_hello"
#define STT_SETTING_METHOD_INITIALIZE		"stt_setting_method_initialize"
#define STT_SETTING_METHOD_FINALIZE		"stt_setting_method_finalilze"
#define STT_SETTING_METHOD_GET_ENGINE_LIST	"stt_setting_method_get_engine_list"
#define STT_SETTING_METHOD_GET_ENGINE		"stt_setting_method_get_engine"
#define STT_SETTING_METHOD_SET_ENGINE		"stt_setting_method_set_engine"
#define STT_SETTING_METHOD_GET_LANG_LIST	"stt_setting_method_get_lang_list"
#define STT_SETTING_METHOD_GET_DEFAULT_LANG	"stt_setting_method_get_default_lang"
#define STT_SETTING_METHOD_SET_DEFAULT_LANG	"stt_setting_method_set_default_lang"
#define STT_SETTING_METHOD_GET_PROFANITY	"stt_setting_method_get_profanity"
#define STT_SETTING_METHOD_SET_PROFANITY	"stt_setting_method_set_profanity"
#define STT_SETTING_METHOD_GET_PUNCTUATION	"stt_setting_method_get_punctuation"
#define STT_SETTING_METHOD_SET_PUNCTUATION	"stt_setting_method_set_punctuation"
#define STT_SETTING_METHOD_GET_SILENCE		"stt_setting_method_get_silence_detection"
#define STT_SETTING_METHOD_SET_SILENCE		"stt_setting_method_set_silence_detection"
#define STT_SETTING_METHOD_GET_ENGINE_SETTING	"stt_setting_method_get_engine_setting"
#define STT_SETTING_METHOD_SET_ENGINE_SETTING	"stt_setting_method_set_engine_setting"

/******************************************************************************************
* Temp file for audio volume
*******************************************************************************************/

#define STT_AUDIO_VOLUME_PATH			"/tmp/stt_vol"

#ifdef __cplusplus
}
#endif

#endif /* __STT_DEFS_H__ */
