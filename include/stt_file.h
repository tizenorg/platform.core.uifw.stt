/*
 * Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __STT_FILE_H__
#define __STT_FILE_H__

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup STT_FILE_MODULE
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief Enumerations of error codes.
*/
typedef enum {
	STT_FILE_ERROR_NONE			= 0,			/**< Successful */
	STT_FILE_ERROR_OUT_OF_MEMORY		= -ENOMEM,		/**< Out of Memory */
	STT_FILE_ERROR_IO_ERROR			= -EIO,			/**< I/O error */
	STT_FILE_ERROR_INVALID_PARAMETER	= -EINVAL,		/**< Invalid parameter */
	STT_FILE_ERROR_OUT_OF_NETWORK		= -ENETDOWN,		/**< Out of network */
	STT_FILE_ERROR_INVALID_STATE		= -0x0100000 | 0x31,	/**< Invalid state */
	STT_FILE_ERROR_INVALID_LANGUAGE		= -0x0100000 | 0x32,	/**< Invalid language */
	STT_FILE_ERROR_ENGINE_NOT_FOUND		= -0x0100000 | 0x33,	/**< No available engine  */
	STT_FILE_ERROR_OPERATION_FAILED		= -0x0100000 | 0x34,	/**< Operation failed  */
	STT_FILE_ERROR_NOT_SUPPORTED_FEATURE	= -0x0100000 | 0x35,	/**< Not supported feature of current engine */
	STT_FILE_ERROR_NOT_AGREE_SERVICE	= -0x0100000 | 0x36	/**< Not agreed service of engine*/
}stt_file_error_e;

/**
* @brief Enumerations of state.
*/
typedef enum {
	STT_FILE_STATE_NONE		= 0,	/**< 'NONE' state */
	STT_FILE_STATE_READY		= 1,	/**< 'READY' state */
	STT_FILE_STATE_PROCESSING	= 2,	/**< 'PROCESSING' state */
}stt_file_state_e;

/**
* @brief Enumerations of audio type.
*/
typedef enum {
	STT_FILE_AUDIO_TYPE_RAW_S16 = 0,	/**< Signed 16-bit audio sample */
	STT_FILE_AUDIO_TYPE_RAW_U8,		/**< Unsigned 8-bit audio sample */
	STT_FILE_AUDIO_TYPE_MAX
}stt_file_audio_type_e;

/**
* @brief Enumerations of result event.
*/
typedef enum {
	STT_FILE_RESULT_EVENT_FINAL_RESULT = 0,	/**< Event when the recognition full or last result is ready  */
	STT_FILE_RESULT_EVENT_PARTIAL_RESULT,	/**< Event when the recognition partial result is ready  */
	STT_FILE_RESULT_EVENT_ERROR		/**< Event when the recognition has failed */
}stt_file_result_event_e;

/**
* @brief Enumerations of result time callback event.
*/
typedef enum {
	STT_FILE_RESULT_TIME_EVENT_BEGINNING = 0,	/**< Event when the token is beginning type */
	STT_FILE_RESULT_TIME_EVENT_MIDDLE = 1,		/**< Event when the token is middle type */
	STT_FILE_RESULT_TIME_EVENT_END = 2		/**< Event when the token is end type */
}stt_file_result_time_event_e;

/**
* @brief Recognition type : Continuous free dictation.
*/
#define STT_RECOGNITION_TYPE_FREE_PARTIAL	"stt.recognition.type.FREE.PARTIAL"


/**
* @brief Called to get a engine information.
*
* @param[in] engine_id Engine id.
* @param[in] engine_name Engine name.
* @param[in] user_data User data passed from the stt_file_setting_foreach_supported_engines().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre stt_file_foreach_supported_engines() will invoke this callback.
*
* @see stt_file_foreach_supported_engines()
*/
typedef bool(*stt_file_supported_engine_cb)(const char* engine_id, const char* engine_name, void* user_data);

/**
* @brief Called when STT gets the recognition result from engine.
*
* @remark After stt_file_start() is called and recognition result is occured, this function is called.
*
* @param[in] event The result event
* @param[in] data Result texts
* @param[in] data_count Result text count
* @param[in] msg Engine message	(e.g. #STT_RESULT_MESSAGE_NONE, #STT_RESULT_MESSAGE_ERROR_TOO_SHORT)
* @param[in] user_data The user data passed from the callback registration function
*
* @pre stt_file_start() will invoke this callback if you register it using stt_file_set_recognition_result_cb().
* @post If this function is called and event is #STT_RESULT_EVENT_FINAL_RESULT, the STT FILE state will be #STT_FILE_STATE_READY.
*
* @see stt_file_set_recognition_result_cb()
* @see stt_file_unset_recognition_result_cb()
*/
typedef void (*stt_file_recognition_result_cb)(stt_file_result_event_e event, const char** data, int data_count,
					  const char* msg, void *user_data);

/**
* @brief Called when STT get the time stamp of recognition result.
*
* @param[in] index The result index
* @param[in] event The token event
* @param[in] text The result text
* @param[in] start_time The start time of result text
* @param[in] end_time The end time of result text
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre stt_file_recognition_result_cb() should be called.
*
* @see stt_file_recognition_result_cb()
*/
typedef bool (*stt_file_result_time_cb)(int index, stt_file_result_time_event_e event, const char* text,
				   long start_time, long end_time, void* user_data);

/**
* @brief Called when the state of STT FILE is changed.
*
* @param[in] previous A previous state
* @param[in] current A current state
* @param[in] user_data The user data passed from the callback registration function.
*
* @pre An application registers this callback using stt_file_set_state_changed_cb() to detect changing state.
*
* @see stt_file_set_state_changed_cb()
* @see stt_file_unset_state_changed_cb()
*/
typedef void (*stt_file_state_changed_cb)(stt_file_state_e previous, stt_file_state_e current, void* user_data);

/**
* @brief Called to retrieve the supported languages.
*
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
*		followed by ISO 639-1 for the two-letter language code. \n
*		For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre stt_file_foreach_supported_languages() will invoke this callback.
*
* @see stt_file_foreach_supported_languages()
*/
typedef bool (*stt_file_supported_language_cb)(const char* language, void* user_data);


/**
* @brief Initialize STT FILE.
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_FILE_ERROR_ENGINE_NOT_FOUND Engine not found
*
* @pre The state should be #STT_FILE_STATE_NONE.
* @post If this function is called, the STT state will be #STT_FILE_STATE_READY.
*
* @see stt_file_deinitialize(void)
*/
int stt_file_initialize(void);

/**
* @brief Deinitialize STT FILE.
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_FILE_STATE_READY.
* @post If this function is called, the STT FILE state will be #STT_FILE_STATE_NONE.
*
* @see stt_file_initialize(void)
*/
int stt_file_deinitialize(void);

/**
* @brief Gets the current state.
*
* @param[out] state The current state of STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see stt_file_state_changed_cb()
*/
int stt_file_get_state(stt_file_state_e* state);

/**
* @brief Retrieve supported engine informations using callback function.
*
* @param[in] callback The callback function to invoke
* @param[in] user_data The user data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_FILE_ERROR_NONE Success.
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_FILE_ERROR_INVALID_STATE STT Not initialized.
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure.
*
* @pre The state should be #STT_FILE_STATE_READY.
* @post	This function invokes stt_file_supported_engine_cb() repeatedly for getting engine information.
*
* @see stt_file_supported_engine_cb()
*/
int stt_file_foreach_supported_engines(stt_file_supported_engine_cb callback, void* user_data);

/**
* @brief Get current engine id.
*
* @remark If the function is success, @a engine_id must be released with free() by you.
*
* @param[out] engine_id engine id.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_FILE_ERROR_NONE Success.
* @retval #STT_FILE_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state.
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure.
*
* @pre The state should be #STT_FILE_STATE_READY.
*
* @see stt_file_foreach_supported_engines()
* @see stt_file_set_engine()
*/
int stt_file_get_engine(char** engine_id);

/**
* @brief Set engine id.
*
* @param[in] engine_id engine id.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STT_FILE_ERROR_NONE Success.
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state.
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure.
*
* @pre The state should be #STT_FILE_STATE_READY.
*
* @see stt_file_get_engine()
*/
int stt_file_set_engine(const char* engine_id);

/**
* @brief Retrieves all supported languages of current engine using callback function.
*
* @param[in] callback The callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_FILE_STATE_READY.
* @post	This function invokes stt_file_supported_language_cb() repeatedly for getting languages.
*
* @see stt_file_supported_language_cb()
* @see stt_file_get_default_language()
*/
int stt_file_foreach_supported_languages(stt_file_supported_language_cb callback, void* user_data);


/**
* @brief Starts file recognition asynchronously.
*
* @param[in] language The language selected from stt_file_foreach_supported_languages()
* @param[in] type The type for recognition (e.g. #STT_RECOGNITION_TYPE_FREE_PARTIAL)
* @param[in] filepath PCM filepath for recognition
* @param[in] audio_type audio type of file
* @param[in] sample_rate sample rate of file
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_FILE_ERROR_INVALID_LANGUAGE Invalid language
* @retval #STT_FILE_ERROR_OUT_OF_NETWORK Out of network
*
* @pre The state should be #STT_FILE_STATE_READY.
* @post It will invoke stt_file_state_changed_cb(), if you register a callback with stt_file_state_changed_cb(). \n
* If this function succeeds, the STT state will be #STT_FILE_STATE_PROCESSING.
*
* @see stt_file_cancel()
*/
int stt_file_start(const char* language, const char* type, const char* filepath,
			stt_file_audio_type_e audio_type, int sample_rate);

/**
* @brief Cancels processing recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #STT_FILE_STATE_PROCESSING.
* @post It will invoke stt_file_state_changed_cb(), if you register a callback with stt_file_state_changed_cb(). \n
* If this function succeeds, the STT state will be #STT_FILE_STATE_READY.
*
* @see stt_file_start()
*/
int stt_file_cancel(void);


/**
* @brief Retrieves time stamp of current recognition result using callback function.
*
* @param[in] callback The callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_FILE_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre This function should be called in stt_file_recognition_result_cb().
* @post	This function invokes stt_file_result_time_cb() repeatedly for getting time information.
*
* @see stt_file_result_time_cb()
* @see stt_file_recognition_result_cb()
*/
int stt_file_foreach_detailed_result(stt_file_result_time_cb callback, void* user_data);

/**
* @brief Registers a callback function for getting recognition result.
*
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_FILE_STATE_READY.
*
* @see stt_file_recognition_result_cb()
* @see stt_file_unset_recognition_result_cb()
*/
int stt_file_set_recognition_result_cb(stt_file_recognition_result_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_FILE_STATE_READY.
*
* @see stt_file_set_recognition_result_cb()
*/
int stt_file_unset_recognition_result_cb(void);

/**
* @brief Registers a callback function to be called when STT FILE state changes.
*
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_FILE_STATE_READY.
*
* @see stt_file_state_changed_cb()
* @see stt_file_unset_state_changed_cb()
*/
int stt_file_set_state_changed_cb(stt_file_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_FILE_ERROR_NONE Successful
* @retval #STT_FILE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_FILE_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_FILE_STATE_READY.
*
* @see stt_file_set_state_changed_cb()
*/
int stt_file_unset_state_changed_cb(void);


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __STT_FILE_H__ */

