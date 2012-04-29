/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup CAPI_UIX_STT_MODULE
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
	STT_ERROR_NONE			= 0,		/**< Successful */
	STT_ERROR_OUT_OF_MEMORY		= -ENOMEM,	/**< Out of Memory */
	STT_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	STT_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	STT_ERROR_TIMED_OUT		= -ETIMEDOUT,	/**< No answer from the daemon */
	STT_ERROR_RECORDER_BUSY		= -EBUSY,	/**< Busy recorder */
	STT_ERROR_OUT_OF_NETWORK	= -ENETDOWN,	/**< Out of network */
	STT_ERROR_INVALID_STATE		= -0x0100031,	/**< Invalid state */
	STT_ERROR_INVALID_LANGUAGE	= -0x0100032,	/**< Invalid language */
	STT_ERROR_ENGINE_NOT_FOUND	= -0x0100033,	/**< No available engine  */	
	STT_ERROR_OPERATION_FAILED	= -0x0100034,	/**< Operation failed  */
	STT_ERROR_NOT_SUPPORTED_FEATURE	= -0x0100035	/**< Not supported feature of current engine */
}stt_error_e;

/** 
* @brief Recognition type : free form dictation or default type.
*/
#define STT_RECOGNITION_TYPE_FREE			"stt.recognition.type.FREE"

/** 
* @brief Recognition type : web search. 
*/
#define STT_RECOGNITION_TYPE_WEB_SEARCH			"stt.recognition.type.WEB_SEARCH"

/** 
* @brief Recognition type : all voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND			"stt.recognition.type.COMMAND"

/** 
* @brief Recognition type : call of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_CALL		"stt.recognition.type.COMMAND.CALL"

/** 
* @brief Recognition type : music of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_MUSIC		"stt.recognition.type.COMMAND.MUSIC"

/** 
* @brief Recognition type : web search of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_WEB_SEARCH		"stt.recognition.type.COMMAND.WEB_SEARCH"

/** 
* @brief Recognition type : schedule of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_SCHEDULE		"stt.recognition.type.COMMAND.SCHEDULE"

/** 
* @brief Recognition type : search of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_SEARCH		"stt.recognition.type.COMMAND.SEARCH"

/** 
* @brief Recognition type : contact of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_CONTACT		"stt.recognition.type.COMMAND.CONTACT"

/** 
* @brief Recognition type : social of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_SOCIAL		"stt.recognition.type.COMMAND.SOCIAL"

/** 
* @brief Recognition type : message of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_MESSAGE		"stt.recognition.type.COMMAND.MESSAGE"

/** 
* @brief Recognition type : email of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_EMAIL		"stt.recognition.type.COMMAND.EMAIL"

/** 
* @brief Recognition type : memo of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_MEMO		"stt.recognition.type.COMMAND.MEMO"

/** 
* @brief Recognition type : alarm of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_ALARM		"stt.recognition.type.COMMAND.ALARM"

/** 
* @brief Recognition type : application of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_APPLICATION	"stt.recognition.type.COMMAND.APPLICATION"

/** 
* @brief Recognition type : driving mode of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_DRIVING_MODE	"stt.recognition.type.COMMAND.DRIVING_MODE"

/** 
* @brief Recognition type : navigation of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_NAVIGATION		"stt.recognition.type.COMMAND.NAVIGATION"

/** 
* @brief Recognition type : text-to-speech of voice commands 
*/
#define STT_RECOGNITION_TYPE_COMMAND_TTS		"stt.recognition.type.COMMAND.TTS"

/** 
* @brief Result message : None message
*/
#define STT_RESULT_MESSAGE_NONE			"stt.result.message.none"

/** 
* @brief Result warning message : The speech has started too soon
*/
#define STT_RESULT_MESSAGE_WARNING_TOO_SOON	"stt.result.message.warning.too.soon"

/** 
* @brief Result warning message : The speech is too short
*/
#define STT_RESULT_MESSAGE_WARNING_TOO_SHORT	"stt.result.message.warning.too.short"

/** 
* @brief Result warning message : The speech is too long
*/
#define STT_RESULT_MESSAGE_WARNING_TOO_LONG	"stt.result.message.warning.too.long"

/** 
* @brief Result warning message : The speech is too quiet to listen
*/
#define STT_RESULT_MESSAGE_WARNING_TOO_QUIET	"stt.result.message.warning.too.quiet"

/** 
* @brief Result warning message : The speech is too loud to listen
*/
#define STT_RESULT_MESSAGE_WARNING_TOO_LOUD	"stt.result.message.warning.too.loud"

/** 
* @brief Result warning message : The speech is too fast to listen
*/
#define STT_RESULT_MESSAGE_WARNING_TOO_FAST	"stt.result.message.warning.too.fast"

/** 
* @brief Result error message : Recognition was failed because the speech started too soon
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_SOON	"stt.result.message.error.too.soon"

/** 
* @brief Result error message : Recognition was failed because the speech started too short
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_SHORT	"stt.result.message.error.too.short"

/** 
* @brief Result error message : Recognition was failed because the speech started too long
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_LONG	"stt.result.message.error.too.long"

/** 
* @brief Result error message : Recognition was failed because the speech started too quiet to listen
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_QUIET	"stt.result.message.error.too.quiet"

/** 
* @brief Result error message : Recognition was failed because the speech started too loud to listen 
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_LOUD	"stt.result.message.error.too.loud"

/** 
* @brief Result error message : Recognition was failed because the speech started too fast to listen
*/
#define STT_RESULT_MESSAGE_ERROR_TOO_FAST	"stt.result.message.error.too.fast"


/** 
* @brief Enumerations of state.
*/
typedef enum {
	STT_STATE_READY = 0,			/**< 'READY' state */
	STT_STATE_RECORDING,			/**< 'RECORDING' state */
	STT_STATE_PROCESSING			/**< 'PROCESSING' state*/
}stt_state_e;

/** 
* @brief Enumerations of profanity type.
*/
typedef enum {
	STT_OPTION_PROFANITY_FALSE = 0,		/**< Profanity type - False */
	STT_OPTION_PROFANITY_TRUE = 1,		/**< Profanity type - True */
	STT_OPTION_PROFANITY_AUTO = 2		/**< Profanity type - Auto */
}stt_option_profanity_e;

/** 
* @brief Enumerations of punctuation type.
*/
typedef enum {
	STT_OPTION_PUNCTUATION_FALSE = 0,	/**< Punctuation type - False */
	STT_OPTION_PUNCTUATION_TRUE = 1,	/**< Punctuation type - True */
	STT_OPTION_PUNCTUATION_AUTO = 2		/**< Punctuation type - Auto */
}stt_option_punctuation_e;

/** 
* @brief Enumerations of silence detection type.
*/
typedef enum {
	STT_OPTION_SILENCE_DETECTION_FALSE = 0,	/**< Silence detection type - False */
	STT_OPTION_SILENCE_DETECTION_TRUE = 1,	/**< Silence detection type - True */
	STT_OPTION_SILENCE_DETECTION_AUTO = 2	/**< Silence detection type - Auto */	
}stt_option_silence_detection_e;

/** 
* @brief A structure of handle for STT
*/
typedef struct stt_s *stt_h;

/**
* @brief Called when STT gets the recognition result from engine after the application call stt_stop().
*
* @remark If results of recognition is valid after stt_stop() is called or silence is detected from recording, 
*	this function is called. 
*
* @param[in] stt The handle for STT
* @param[in] type Recognition type (e.g. #STT_RECOGNITION_TYPE_FREE, #STT_RECOGNITION_TYPE_COMMAND)
* @param[in] data Result texts
* @param[in] data_count Result text count
* @param[in] msg Engine message	(e.g. #STT_RESULT_MESSAGE_WARNING_TOO_SOON, #STT_RESULT_MESSAGE_ERROR_TOO_SHORT)
* @param[in] user_data The user data passed from the callback registration function
*
* @pre stt_stop() will invoke this callback if you register it using stt_set_result_cb().
* @post If this function is called, the STT state will be #STT_STATE_READY.
*
* @see stt_stop()
* @see stt_set_result_cb()
* @see stt_unset_result_cb()
*/
typedef void (*stt_result_cb)(stt_h stt, const char* type, const char** data, int data_count, const char* msg, void *user_data);

/**
* @brief Called when STT gets recognition the partial result from engine after the application calls stt_start().
*
* @remark If a current engine supports partial result of recognition, this function can be called.
*
* @param[in] stt The handle for STT
* @param[in] data Result data
* @param[in] user_data The user data passed from the callback registration function
*
* @pre stt_start() will invoke this callback if you register it using stt_set_partial_result_cb().
*
* @see stt_start()
* @see stt_set_partial_result_cb()
* @see stt_unset_partial_result_cb()
* @see stt_is_partial_result_supported()
*/
typedef void (*stt_partial_result_cb)(stt_h stt, const char* data, void *user_data);

/**
* @brief Called when the state of STT is changed. 
*
* @param[in] stt The handle for STT
* @param[in] previous A previous state
* @param[in] current A current state
* @param[in] user_data The user data passed from the callback registration function.
*
* @pre An application registers this callback using stt_set_state_changed_cb() to detect changing state.
*
* @see stt_start()
* @see stt_stop()
* @see stt_cancel()
* @see stt_result_cb()
* @see stt_set_state_changed_cb()
* @see stt_unset_state_changed_cb()
*/
typedef void (*stt_state_changed_cb)(stt_h stt, stt_state_e previous, stt_state_e current, void* user_data);

/**
* @brief Called when error occurred. 
*
* @param[in] stt The handle for STT
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
*
* @param[in] stt The handle for STT
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
* followed by ISO 639-1 for the two-letter language code. \n
* For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre stt_foreach_supported_languages() will invoke this callback. 
*
* @see stt_foreach_supported_languages()
*/
typedef bool(*stt_supported_language_cb)(stt_h stt, const char* language, void* user_data);


/**
* @brief Creates a handle for STT and connects daemon. 
*
* @param[out] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_TIMED_OUT The daemon is blocked or do not exist
* @retval #STT_ERROR_ENGINE_NOT_FOUND No available engine \n Engine should be installed
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_OUT_OF_MEMORY Out of memory
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
*
* @see stt_destroy()
*/
int stt_create(stt_h* stt);

/**
* @brief Destroys the handle and disconnects the daemon.
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see stt_create()
*/
int stt_destroy(stt_h stt);

/**
* @brief Retrieves all supported languages of current engine using callback function.
*
* @param[in] stt The handle for STT
* @param[in] callback The callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
* @post	This function invokes stt_supported_language_cb() repeatedly for getting languages. 
*
* @see stt_supported_language_cb()
* @see stt_get_default_language()
*/
int stt_foreach_supported_languages(stt_h stt, stt_supported_language_cb callback, void* user_data);

/**
* @brief Gets the default language set by user.
*
* @remark If the function succeeds, @a language must be released with free() by you when you no longer need it.
*
* @param[in] stt The handle for STT
* @param[out] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
* followed by ISO 639-1 for the two-letter language code. \n
* For example, "ko_KR" for Korean, "en_US" for American English.
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_OUT_OF_MEMORY Out of memory
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_foreach_supported_languages()
*/
int stt_get_default_language(stt_h stt, char** language);

/**
* @brief Gets the current state of STT.
*
* @param[in] stt The handle for STT
* @param[out] state The current state of STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see stt_start()
* @see stt_stop()
* @see stt_cancel()
* @see stt_state_changed_cb()
*/
int stt_get_state(stt_h stt, stt_state_e* state);

/**
* @brief Checks whether the partial results can be returned while a recognition is in process.
*
* @param[in] stt The handle for STT
* @param[out] partial_result The partial result status : (@c true = supported, @c false = not supported)
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_partial_result_cb()
*/
int stt_is_partial_result_supported(stt_h stt, bool* partial_result);

/**
* @brief Sets profanity filter.
*
* @param[in] stt The handle for STT
* @param[in] type The option type
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_NOT_SUPPORTED_FEATURE Not supported feature of current engine
*
* @pre The state should be #STT_STATE_READY.
*/
int stt_set_profanity_filter(stt_h stt, stt_option_profanity_e type);

/**
* @brief Sets punctuation override.
*
* @param[in] stt The handle for STT
* @param[in] type The option type
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_NOT_SUPPORTED_FEATURE Not supported feature of current engine
*
* @pre The state should be #STT_STATE_READY.
*/
int stt_set_punctuation_override(stt_h stt, stt_option_punctuation_e type);

/**
* @brief Sets silence detection.
*
* @param[in] stt The handle for STT
* @param[in] type The option type
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_NOT_SUPPORTED_FEATURE Not supported feature of current engine
*
* @pre The state should be #STT_STATE_READY.
*/
int stt_set_silence_detection(stt_h stt, stt_option_silence_detection_e type);

/**
* @brief Starts recording and recognition.
*
* @remark This function starts recording in the daemon and sending recording data to engine. \n
* This work continues until stt_stop(), stt_cancel() or silence detected.
*
* @param[in] stt The handle for STT
* @param[in] language The language selected from stt_foreach_supported_languages()
* @param[in] type The type for recognition (e.g. #STT_RECOGNITION_TYPE_FREE, #STT_RECOGNITION_TYPE_WEB_SEARCH)
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
* @retval #STT_ERROR_RECORDER_BUSY Recorder busy
* @retval #STT_ERROR_INVALID_LANGUAGE Invalid language
*
* @pre The state should be #STT_STATE_READY.
* @post It will invoke stt_state_changed_cb(), if you register a callback with stt_state_changed_cb(). \n
* If this function succeeds, the STT state will be #STT_STATE_RECORDING.
*
* @see stt_stop()
* @see stt_cancel()
* @see stt_state_changed_cb()
* @see stt_error_cb()
*/
int stt_start(stt_h stt, const char* language, const char* type);

/**
* @brief Finishes recording and starts recognition processing in engine.
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_OUT_OF_MEMORY Not enough memory
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #STT_STATE_RECORDING.
* @post It will invoke stt_state_changed_cb(), if you register a callback with stt_state_changed_cb(). \n
* If this function succeeds, the STT state will be #STT_STATE_PROCESSING. \n
* After processing of engine, stt_result_cb() is called.
*
* @see stt_start()
* @see stt_cancel()
* @see stt_state_changed_cb()
* @see stt_error_cb()
*/
int stt_stop(stt_h stt);

/**
* @brief Cancels processing recognition and recording.
*
* @remark This function cancels recording and engine cancels recognition processing. \n
* After successful cancel, stt_state_changed_cb() is called otherwise if error is occurred, stt_error_cb() is called. 
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_OUT_OF_MEMORY Not enough memory
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #STT_STATE_RECORDING or #STT_STATE_PROCESSING.
* @post It will invoke stt_state_changed_cb(), if you register a callback with stt_state_changed_cb(). \n
* If this function succeeds, the STT state will be #STT_STATE_READY.
*
* @see stt_start()
* @see stt_stop()
* @see stt_state_changed_cb()
* @see stt_error_cb()
*/
int stt_cancel(stt_h stt);

/**
* @brief Gets the microphone volume during recording.
*	
* @param[in] stt The handle for STT
* @param[out] volume Recording volume
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
* @retval #STT_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #STT_STATE_RECORDING.
* 
* @see stt_start()
*/
int stt_get_recording_volume(stt_h stt, float* volume);

/**
* @brief Registers a callback function for getting recognition result.
*
* @param[in] stt The handle for STT
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_result_cb()
* @see stt_unset_result_cb()
*/
int stt_set_result_cb(stt_h stt, stt_result_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_set_result_cb()
*/
int stt_unset_result_cb(stt_h stt);

/**
* @brief Registers a callback function for getting partial result of recognition.
*
* @param[in] stt The handle for STT
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @see stt_partial_result_cb()
* @see stt_unset_partial_result_cb()
*/
int stt_set_partial_result_cb(stt_h stt, stt_partial_result_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_set_partial_result_cb()
*/
int stt_unset_partial_result_cb(stt_h stt);

/**
* @brief Registers a callback function to be called when STT state changes.
*
* @param[in] stt The handle for STT
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_state_changed_cb()
* @see stt_unset_state_changed_cb()
*/
int stt_set_state_changed_cb(stt_h stt, stt_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_set_state_changed_cb()
*/
int stt_unset_state_changed_cb(stt_h stt);

/**
* @brief Registers a callback function to be called when an error occurred.
*
* @param[in] stt The handle for STT
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_error_cb()
* @see stt_unset_error_cb()
*/
int stt_set_error_cb(stt_h stt, stt_error_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] stt The handle for STT
*
* @return 0 on success, otherwise a negative error value
* @retval #STT_ERROR_NONE Successful
* @retval #STT_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #STT_STATE_READY.
*
* @see stt_set_error_cb()
*/
int stt_unset_error_cb(stt_h stt);


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __STT_H__ */
