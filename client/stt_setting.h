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


#ifndef __STT_SETTING_H_
#define __STT_SETTING_H_

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup STT_SETTING_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/** 
* @brief Enumerations of error codes.
*/
typedef enum {
	STT_SETTING_ERROR_NONE			= 0,		/**< Success, No error */
	STT_SETTING_ERROR_OUT_OF_MEMORY		= -ENOMEM,	/**< Out of Memory */
	STT_SETTING_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	STT_SETTING_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	STT_SETTING_ERROR_TIMED_OUT		= -ETIMEDOUT,	/**< No answer from the daemon */
	STT_SETTING_ERROR_OUT_OF_NETWORK	= -ENETDOWN,	/**< Out of network */
	STT_SETTING_ERROR_INVALID_STATE		= -0x0100031,	/**< Invalid state */
	STT_SETTING_ERROR_INVALID_LANGUAGE	= -0x0100032,	/**< Invalid language */
	STT_SETTING_ERROR_ENGINE_NOT_FOUND	= -0x0100033,	/**< No available STT-engine  */
	STT_SETTING_ERROR_OPERATION_FAILED	= -0x0100034,	/**< STT daemon failed  */
	STT_SETTING_ERROR_NOT_SUPPORTED_FEATURE	= -0x0100035	/**< Not supported feature of current engine */
}stt_setting_error_e;

/** 
* @brief Enumerations of setting state.
*/
typedef enum {
	STT_SETTING_STATE_NONE = 0,
	STT_SETTING_STATE_READY
} stt_setting_state_e;

/**
* @brief Called to get a engine information.
*
* @param[in] engine_id Engine id.
* @param[in] engine_name engine name.
* @param[in] setting_path gadget path of engine specific setting.
* @param[in] user_data User data passed from the stt_setting_foreach_supported_engines().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre stt_setting_foreach_supported_engines() will invoke this callback. 
*
* @see stt_setting_foreach_supported_engines()
*/
typedef bool(*stt_setting_supported_engine_cb)(const char* engine_id, const char* engine_name, const char* setting_path, void* user_data);

/**
* @brief Called to get a language.
*
* @param[in] engine_id Engine id.
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code
*		followed by ISO 639-1 for the two-letter language code. 
*		For example, "ko_KR" for Korean, "en_US" for American English..
* @param[in] user_data User data passed from the stt_setting_foreach_supported_languages().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre stt_setting_foreach_supported_languages() will invoke this callback. 
*
* @see stt_setting_foreach_supported_languages()
*/
typedef bool(*stt_setting_supported_language_cb)(const char* engine_id, const char* language, void* user_data);

/**
* @brief Called to get a engine setting.
*
* @param[in] engine_id Engine id.
* @param[in] key Key.
* @param[in] value Value.
* @param[in] user_data User data passed from the stt_setting_foreach_engine_settings().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre stt_setting_foreach_engine_settings() will invoke this callback. 
*
* @see stt_setting_foreach_engine_settings()
*/
typedef bool(*stt_setting_engine_setting_cb)(const char* engine_id, const char* key, const char* value, void* user_data);

/**
* @brief Called to initialize setting.
*
* @param[in] state Current state.
* @param[in] reason Error reason.
* @param[in] user_data User data passed from the stt_setting_initialize_async().
*
* @pre stt_setting_initialize_async() will invoke this callback. 
*
* @see stt_setting_initialize_async()
*/
typedef void(*stt_setting_initialized_cb)(stt_setting_state_e state, stt_setting_error_e reason, void* user_data);

/**
* @brief Initialize STT setting and connect to stt-daemon.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_TIMED_OUT stt daemon is blocked or stt daemon do not exist.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT setting has Already been initialized. 
* @retval #STT_SETTING_ERROR_ENGINE_NOT_FOUND No available stt-engine. Engine should be installed.
*
* @see stt_setting_finalize()
*/
int stt_setting_initialize(void);
int stt_setting_initialize_async(stt_setting_initialized_cb callback, void* user_data);

/**
* @brief finalize stt setting and disconnect to stt-daemon. 
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_initialize()
*/
int stt_setting_finalize(void);

/**
* @brief Retrieve supported engine informations using callback function.
*
* @param[in] callback callback function
* @param[in] user_data User data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @post	This function invokes stt_setting_supported_engine_cb() repeatedly for getting engine information. 
*
* @see stt_setting_supported_engine_cb()
*/
int stt_setting_foreach_supported_engines(stt_setting_supported_engine_cb callback, void* user_data);

/**
* @brief Get current engine id.
*
* @remark If the function is success, @a engine_id must be released with free() by you.
*
* @param[out] engine_id engine id.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_set_engine()
*/
int stt_setting_get_engine(char** engine_id);

/**
* @brief Set current engine id.
*
* @param[in] engine_id engine id.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_get_engine()
*/
int stt_setting_set_engine(const char* engine_id);

/**
* @brief Get supported languages of current engine.
*
* @param[in] callback callback function.
* @param[in] user_data User data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @post	This function invokes stt_setting_supported_language_cb() repeatedly for getting supported languages. 
*
* @see stt_setting_supported_language_cb()
*/
int stt_setting_foreach_supported_languages(stt_setting_supported_language_cb callback, void* user_data);

/**
* @brief Get a default language of current engine.
*
* @remark If the function is success, @a language must be released with free() by you.
*
* @param[out] language current language
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_set_default_language()
*/
int stt_setting_get_default_language(char** language);

/**
* @brief Set a default language of current engine.
*
* @param[in] language language
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_get_default_language()
*/
int stt_setting_set_default_language(const char* language);

/**
* @brief Get profanity filter 
*
* @param[out] value Value. 
*
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_set_profanity_filter()
*/
int stt_setting_get_profanity_filter(bool* value);

/**
* @brief Set profanity filter.
*
* @param[in] value Value.
*
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_get_profanity_filter()
*/
int stt_setting_set_profanity_filter(const bool value);

/**
* @brief Get punctuation override.
*
* @param[out] value Value. 
*
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_set_punctuation_override()
*/
int stt_setting_get_punctuation_override(bool* value);

/**
* @brief Set punctuation override.
*
* @param[in] value Value.
*
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_get_punctuation_override()
*/
int stt_setting_set_punctuation_override(const bool value);

/**
* @brief Get silence detection.
*
* @param[out] value Value. 
*
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_set_silence_detection()
*/
int stt_setting_get_silence_detection(bool* value);

/**
* @brief Set silence detection.
*
* @param[in] value Value. 
*
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_get_silence_detection()
*/
int stt_setting_set_silence_detection(const bool value);

/**
* @brief Get setting information of current engine.
*
* @param[in] callback callback function
* @param[in] user_data User data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @post	This function invokes stt_setting_engine_setting_cb() repeatedly for getting engine settings. 
*
* @see stt_setting_engine_setting_cb()
*/
int stt_setting_foreach_engine_settings(stt_setting_engine_setting_cb callback, void* user_data);

/**
* @brief Set setting information.
*
* @param[in] key Key.
* @param[in] value Value.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_SETTING_ERROR_NONE Success.
* @retval #STT_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_SETTING_ERROR_INVALID_STATE STT Not initialized. 
* @retval #STT_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see stt_setting_foreach_engine_settings()
*/
int stt_setting_set_engine_setting(const char* key, const char* value);


#ifdef __cplusplus
}
#endif

/**
* @}@}
*/

#endif /* __STT_SETTING_H_ */
