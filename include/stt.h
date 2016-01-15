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

#ifndef __STT_H__
#define __STT_H__

#include <tizen.h>

/**
 * @file stt.h
 */

/**
* @addtogroup CAPI_UIX_STT_MODULE
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Enumerations for error codes.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	STT_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Successful */
	STT_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	STT_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	STT_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	STT_ERROR_TIMED_OUT		= TIZEN_ERROR_TIMED_OUT,	/**< No answer from the daemon */
	STT_ERROR_RECORDER_BUSY		= TIZEN_ERROR_RESOURCE_BUSY,	/**< Device or resource busy */
	STT_ERROR_OUT_OF_NETWORK	= TIZEN_ERROR_NETWORK_DOWN,	/**< Network is down */
	STT_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	STT_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,	/**< STT NOT supported */
	STT_ERROR_INVALID_STATE		= TIZEN_ERROR_STT | 0x01,	/**< Invalid state */
	STT_ERROR_INVALID_LANGUAGE	= TIZEN_ERROR_STT | 0x02,	/**< Invalid language */
	STT_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_STT | 0x03,	/**< No available engine  */
	STT_ERROR_OPERATION_FAILED	= TIZEN_ERROR_STT | 0x04,	/**< Operation failed  */
	STT_ERROR_NOT_SUPPORTED_FEATURE	= TIZEN_ERROR_STT | 0x05	/**< Not supported feature of current engine */
}stt_error_e;

/**
 * @brief Definition for free form dictation and default type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RECOGNITION_TYPE_FREE		"stt.recognition.type.FREE"

/**
 * @brief Definition for continuous free dictation.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RECOGNITION_TYPE_FREE_PARTIAL	"stt.recognition.type.FREE.PARTIAL"

/**
 * @brief Definition for search.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RECOGNITION_TYPE_SEARCH		"stt.recognition.type.SEARCH"

/**
 * @brief Definition for web search.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RECOGNITION_TYPE_WEB_SEARCH		"stt.recognition.type.WEB_SEARCH"

/**
 * @brief Definition for map.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RECOGNITION_TYPE_MAP		"stt.recognition.type.MAP"

/**
 * @brief Definition for none message.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_NONE			"stt.result.message.none"

/**
 * @brief Definition for failed recognition because the speech started too soon.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_SOON	"stt.result.message.error.too.soon"

/**
 * @brief Definition for failed recognition because the speech is too short.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_SHORT	"stt.result.message.error.too.short"

/**
 * @brief Definition for failed recognition because the speech is too long.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_LONG	"stt.result.message.error.too.long"

/**
 * @brief Definition for failed recognition because the speech is too quiet to listen.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_QUIET	"stt.result.message.error.too.quiet"

/**
 * @brief Definition for failed recognition because the speech is too loud to listen.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_LOUD	"stt.result.message.error.too.loud"

/**
 * @brief Definition for failed recognition because the speech is too fast to listen.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_FAST	"stt.result.message.error.too.fast"


/**
 * @brief Enumeration for state.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	STT_STATE_CREATED	= 0,		/**< 'CREATED' state */
	STT_STATE_READY		= 1,		/**< 'READY' state */
	STT_STATE_RECORDING	= 2,		/**< 'RECORDING' state */
	STT_STATE_PROCESSING	= 3		/**< 'PROCESSING' state*/
}stt_state_e;

/**
 * @brief Enumeration for result event.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	STT_RESULT_EVENT_FINAL_RESULT = 0,	/**< Event when the recognition full or last result is ready  */
	STT_RESULT_EVENT_PARTIAL_RESULT,	/**< Event when the recognition partial result is ready  */
	STT_RESULT_EVENT_ERROR			/**< Event when the recognition has failed */
}stt_result_event_e;

/**
 * @brief Enumeration for result time callback event.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	STT_RESULT_TIME_EVENT_BEGINNING = 0,	/**< Event when the token is beginning type */
	STT_RESULT_TIME_EVENT_MIDDLE = 1,	/**< Event when the token is middle type */
	STT_RESULT_TIME_EVENT_END = 2		/**< Event when the token is end type */
}stt_result_time_event_e;

/**
 * @brief Enumeration for silence detection type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	STT_OPTION_SILENCE_DETECTION_FALSE = 0,	/**< Silence detection type - False */
	STT_OPTION_SILENCE_DETECTION_TRUE = 1,	/**< Silence detection type - True */
	STT_OPTION_SILENCE_DETECTION_AUTO = 2	/**< Silence detection type - Auto */
}stt_option_silence_detection_e;

/**
 * @brief A structure of STT handler.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef struct stt_s *stt_h;

/**
 * @brief Called to get the engine information.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] engine_id Engine id
 * @param[in] engine_name Engine name
 * @param[in] user_data User data passed from the stt_setting_foreach_supported_engines()
 *
 * @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop
 * @pre stt_foreach_supported_engines() will invoke this callback.
 *
 * @see stt_foreach_supported_engines()
*/
typedef bool(*stt_supported_engine_cb)(stt_h stt, const char* engine_id, const char* engine_name, void* user_data);

/**
 * @brief Called when STT gets the recognition result from the engine.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @remarks After stt_stop() is called, silence is detected from recording, or partial result is occured,
 *	this function is called.
 *
 * @param[in] stt The STT handle
 * @param[in] event The result event
 * @param[in] data Result texts
 * @param[in] data_count Result text count
 * @param[in] msg Engine message (e.g. #STT_RESULT_MESSAGE_NONE, #STT_RESULT_MESSAGE_ERROR_TOO_SHORT)
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre stt_stop() will invoke this callback if you register it using stt_set_result_cb().
 * @post If this function is called and event is #STT_RESULT_EVENT_FINAL_RESULT, the STT state will be #STT_STATE_READY.
 *
 * @see stt_stop()
 * @see stt_set_recognition_result_cb()
 * @see stt_unset_recognition_result_cb()
*/
typedef void (*stt_recognition_result_cb)(stt_h stt, stt_result_event_e event, const char** data, int data_count,
					  const char* msg, void *user_data);

/**
 * @brief Called when STT get the result time stamp in free partial type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] stt The STT handle
 * @param[in] index The result index
 * @param[in] event The token event
 * @param[in] text The result text
 * @param[in] start_time The start time of result text
 * @param[in] end_time The end time of result text
 * @param[in] user_data The user data passed from the foreach function
 *
 * @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
 *
 * @pre stt_recognition_result_cb() should be called.
 *
 * @see stt_recognition_result_cb()
*/
typedef bool (*stt_result_time_cb)(stt_h stt, int index, stt_result_time_event_e event, const char* text,
				   long start_time, long end_time, void* user_data);

/**
 * @brief Called when the state of STT is changed.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] stt The STT handle
 * @param[in] previous A previous state
 * @param[in] current A current state
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre An application registers this callback using stt_set_state_changed_cb() to detect changing state.
 *
 * @see stt_set_state_changed_cb()
 * @see stt_unset_state_changed_cb()
*/
typedef void (*stt_state_changed_cb)(stt_h stt, stt_state_e previous, stt_state_e current, void* user_data);

/**
 * @brief Called when an error occurs.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] stt The STT handle
 * @param[in] reason The error type (e.g. #STT_ERROR_OUT_OF_NETWORK, #STT_ERROR_IO_ERROR)
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre An application registers this callback using stt_set_error_cb() to detect error.
 *
 * @see stt_set_error_cb()
 * @see stt_unset_error_cb()
*/
typedef void (*stt_error_cb)(stt_h stt, stt_error_e reason, void *user_data);

/**
 * @brief Called to retrieve the supported languages.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @remarks The language is specified as an ISO 3166 alpha-2 two letter country-code followed by ISO 639-1 for the two-letter language code. For example, "ko_KR" for Korean, "en_US" for American English.
 *
 * @param[in] stt The STT handle
 * @param[in] language The language
 * @param[in] user_data The user data passed from the foreach function
 *
 * @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop
 * @pre stt_foreach_supported_languages() will invoke this callback.
 *
 * @see stt_foreach_supported_languages()
*/
typedef bool (*stt_supported_language_cb)(stt_h stt, const char* language, void* user_data);

/**
 * @brief Called when the default language is changed.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] stt The STT handle
 * @param[in] previous_language A previous language
 * @param[in] current_language A current language
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @see stt_set_default_language_changed_cb()
*/
typedef void (*stt_default_language_changed_cb)(stt_h stt, const char* previous_language,
						const char* current_language, void* user_data);

/**
 * @brief Creates a STT handle.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks If the function succeeds, @a stt handle must be released with stt_destroy().
 *
 * @param[out] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @post If this function is called, the STT state will be #STT_STATE_CREATED.
 *
 * @see stt_destroy()
*/
int stt_create(stt_h* stt);

/**
 * @brief Destroys a STT handle.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @see stt_create()
*/
int stt_destroy(stt_h stt);

/**
 * @brief Retrieves supported engine information using a callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to invoke
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Success
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE STT Not initialized
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 * @post This function invokes stt_supported_engine_cb() repeatedly for getting engine information.
 *
 * @see stt_supported_engine_cb()
*/
int stt_foreach_supported_engines(stt_h stt, stt_supported_engine_cb callback, void* user_data);

/**
 * @brief Gets the current engine id.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks If the function is success, @a engine_id must be released using free().
 *
 * @param[in] stt The STT handle
 * @param[out] engine_id Engine id
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Success
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE STT Not initialized
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_set_engine()
*/
int stt_get_engine(stt_h stt, char** engine_id);

/**
 * @brief Sets the engine id.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] engine_id Engine id
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Success
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE STT Not initialized
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_get_engine()
*/
int stt_set_engine(stt_h stt, const char* engine_id);

/**
 * @brief Connects the daemon asynchronously.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 * @post If this function is successful, the STT state will be #STT_STATE_READY. \n
 *	If this function is failed, the error callback is called. (e.g. #STT_ERROR_ENGINE_NOT_FOUND)
 *
 * @see stt_unprepare()
*/
int stt_prepare(stt_h stt);

/**
 * @brief Disconnects the daemon.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
 * @post If this function is called, the STT state will be #STT_STATE_CREATED.
 *
 * @see stt_prepare()
*/
int stt_unprepare(stt_h stt);

/**
 * @brief Retrieves all supported languages of current engine using callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to invoke
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_ENGINE_NOT_FOUND No available engine
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @post This function invokes stt_supported_language_cb() repeatedly for getting languages.
 *
 * @see stt_supported_language_cb()
 * @see stt_get_default_language()
*/
int stt_foreach_supported_languages(stt_h stt, stt_supported_language_cb callback, void* user_data);

/**
 * @brief Gets the default language set by the user.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 * @remarks The language is specified as an ISO 3166 alpha-2 two letter country-code followed by ISO 639-1 for the two-letter language code. \n
 * For example, "ko_KR" for Korean, "en_US" for American English. \n
 * If the function succeeds, @a language must be released using free() when it is no longer required.
 *
 * @param[in] stt The STT handle
 * @param[out] language The language
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @see stt_foreach_supported_languages()
*/
int stt_get_default_language(stt_h stt, char** language);

/**
 * @brief Gets the current STT state.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[out] state The current STT state
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @see stt_start()
 * @see stt_stop()
 * @see stt_cancel()
 * @see stt_state_changed_cb()
*/
int stt_get_state(stt_h stt, stt_state_e* state);

/**
 * @brief Checks whether the recognition type is supported.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] type The type for recognition (e.g. #STT_RECOGNITION_TYPE_FREE, #STT_RECOGNITION_TYPE_FREE_PARTIAL)
 * @param[out] support The result status @c true = supported, @c false = not supported
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
*/
int stt_is_recognition_type_supported(stt_h stt, const char* type, bool* support);

/**
 * @brief Sets the silence detection.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] type The option type
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED_FEATURE Not supported feature of current engine
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
*/
int stt_set_silence_detection(stt_h stt, stt_option_silence_detection_e type);

/**
 * @brief Sets the sound to start recording.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks Sound file type should be wav type.
 *
 * @param[in] stt The STT handle
 * @param[in] filename The sound file path
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
*/
int stt_set_start_sound(stt_h stt, const char* filename);

/**
 * @brief Unsets the sound to start recording.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
*/
int stt_unset_start_sound(stt_h stt);

/**
 * @brief Sets the sound to stop recording.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks Sound file type should be wav type.
 *
 * @param[in] stt The STT handle
 * @param[in] filename The sound file path
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
*/
int stt_set_stop_sound(stt_h stt, const char* filename);

/**
 * @brief Unsets the sound to stop recording.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
*/
int stt_unset_stop_sound(stt_h stt);

/**
 * @brief Starts recording and recognition asynchronously.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks This function starts recording in the daemon and sending recording data to engine. \n
 * This work continues until stt_stop(), stt_cancel() or silence detected by engine.
 *
 * @param[in] stt The STT handle
 * @param[in] language The language selected from stt_foreach_supported_languages()
 * @param[in] type The type for recognition (e.g. #STT_RECOGNITION_TYPE_FREE, #STT_RECOGNITION_TYPE_FREE_PARTIAL)
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_RECORDER_BUSY Recorder busy
 * @retval #STT_ERROR_INVALID_LANGUAGE Invalid language
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_READY.
 * @post It will invoke stt_state_changed_cb(), if you register a callback with stt_state_changed_cb(). \n
 * If this function succeeds, the STT state will be #STT_STATE_RECORDING.
 *
 * @see stt_stop()
 * @see stt_cancel()
 * @see stt_state_changed_cb()
*/
int stt_start(stt_h stt, const char* language, const char* type);

/**
 * @brief Finishes the recording and starts recognition processing in engine asynchronously.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_RECORDING.
 * @post It will invoke stt_state_changed_cb(), if you register a callback with stt_state_changed_cb(). \n
 * If this function succeeds, the STT state will be #STT_STATE_PROCESSING. \n
 * After processing of engine, stt_result_cb() is called.
 *
 * @see stt_start()
 * @see stt_cancel()
 * @see stt_state_changed_cb()
*/
int stt_stop(stt_h stt);

/**
 * @brief Cancels processing recognition and recording asynchronously.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks This function cancels recording and engine cancels recognition processing. \n
 * After successful cancel, stt_state_changed_cb() is called otherwise if error is occurred, stt_error_cb() is called.
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_RECORDING or #STT_STATE_PROCESSING.
 * @post It will invoke stt_state_changed_cb(), if you register a callback with stt_state_changed_cb(). \n
 * If this function succeeds, the STT state will be #STT_STATE_READY.
 *
 * @see stt_start()
 * @see stt_stop()
 * @see stt_state_changed_cb()
*/
int stt_cancel(stt_h stt);

/**
 * @brief Gets the microphone volume during recording.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[out] volume Recording volume
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_RECORDING.
 *
 * @see stt_start()
*/
int stt_get_recording_volume(stt_h stt, float* volume);

/**
 * @brief Retrieves the time stamp of the current recognition result using the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @remarks This function should be called in stt_recognition_result_cb().
 *	After stt_recognition_result_cb(), result data is NOT valid.
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to invoke
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_OPERATION_FAILED Operation failure
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre This function should be called in stt_recognition_result_cb().
 * @post This function invokes stt_result_time_cb() repeatedly for getting time information.
 *
 * @see stt_result_time_cb()
 * @see stt_recognition_result_cb()
*/
int stt_foreach_detailed_result(stt_h stt, stt_result_time_cb callback, void* user_data);

/**
 * @brief Registers a callback function to get the recognition result.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_recognition_result_cb()
 * @see stt_unset_recognition_result_cb()
*/
int stt_set_recognition_result_cb(stt_h stt, stt_recognition_result_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_set_recognition_result_cb()
*/
int stt_unset_recognition_result_cb(stt_h stt);

/**
 * @brief Registers a callback function to be called when STT state changes.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_state_changed_cb()
 * @see stt_unset_state_changed_cb()
*/
int stt_set_state_changed_cb(stt_h stt, stt_state_changed_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_set_state_changed_cb()
*/
int stt_unset_state_changed_cb(stt_h stt);

/**
 * @brief Registers a callback function to be called when an error occurred.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_error_cb()
 * @see stt_unset_error_cb()
*/
int stt_set_error_cb(stt_h stt, stt_error_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_set_error_cb()
*/
int stt_unset_error_cb(stt_h stt);

/**
 * @brief Registers a callback function to detect the default language change.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_default_language_changed_cb()
 * @see stt_unset_default_language_changed_cb()
*/
int stt_set_default_language_changed_cb(stt_h stt, stt_default_language_changed_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 * @privlevel public
 * @privilege %http://tizen.org/privilege/recorder
 *
 * @param[in] stt The STT handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #STT_ERROR_NONE Successful
 * @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #STT_ERROR_INVALID_STATE Invalid state
 * @retval #STT_ERROR_NOT_SUPPORTED STT NOT supported
 *
 * @pre The state should be #STT_STATE_CREATED.
 *
 * @see stt_set_default_language_changed_cb()
*/
int stt_unset_default_language_changed_cb(stt_h stt);

#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __STT_H__ */

