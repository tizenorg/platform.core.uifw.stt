/*
*  Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __STT_ENGINE_MAIN_H__
#define __STT_ENGINE_MAIN_H__

#include <tizen.h>

/**
* @addtogroup CAPI_UIX_STTE_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Enumerations for error codes.
* @since_tizen 3.0
*/
typedef enum {
	STTE_ERROR_NONE				= TIZEN_ERROR_NONE,			/**< Successful */
	STTE_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,		/**< Out of Memory */
	STTE_ERROR_IO_ERROR			= TIZEN_ERROR_IO_ERROR,			/**< I/O error */
	STTE_ERROR_INVALID_PARAMETER		= TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	STTE_ERROR_NETWORK_DOWN			= TIZEN_ERROR_NETWORK_DOWN,		/**< Network down(Out of network) */
	STTE_ERROR_INVALID_STATE		= TIZEN_ERROR_STT | 0x01,		/**< Invalid state */
	STTE_ERROR_INVALID_LANGUAGE		= TIZEN_ERROR_STT | 0x02,		/**< Invalid language */
	STTE_ERROR_OPERATION_FAILED		= TIZEN_ERROR_STT | 0x04,		/**< Operation failed */
	STTE_ERROR_NOT_SUPPORTED_FEATURE	= TIZEN_ERROR_STT | 0x05,		/**< Not supported feature */
	STTE_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,		/**< Not supported */
	STTE_ERROR_PERMISSION_DENIED		= TIZEN_ERROR_PERMISSION_DENIED,	/**< Permission denied */
	STTE_ERROR_RECORDING_TIMED_OUT		= TIZEN_ERROR_STT | 0x06		/**< Recording timed out */
} stte_error_e;

/**
* @brief Enumerations for audio type.
* @since_tizen 3.0
*/
typedef enum {
	STTE_AUDIO_TYPE_PCM_S16_LE = 0,		/**< Signed 16bit audio type, Little endian */
	STTE_AUDIO_TYPE_PCM_U8			/**< Unsigned 8bit audio type */
} stte_audio_type_e;

/**
* @brief Enumerations for callback event.
* @since_tizen 3.0
*/
typedef enum {
	STTE_RESULT_EVENT_FINAL_RESULT = 0,	/**< Event when either the full matched or the final result is delivered */
	STTE_RESULT_EVENT_PARTIAL_RESULT,	/**< Event when the partial matched result is delivered */
	STTE_RESULT_EVENT_ERROR			/**< Event when the recognition has failed */
} stte_result_event_e;

/**
* @brief Enumerations for result time callback event.
* @since_tizen 3.0
*/
typedef enum {
	STTE_RESULT_TIME_EVENT_BEGINNING = 0,	/**< Event when the token is beginning type */
	STTE_RESULT_TIME_EVENT_MIDDLE,		/**< Event when the token is middle type */
	STTE_RESULT_TIME_EVENT_END		/**< Event when the token is end type */
} stte_result_time_event_e;

/**
* @brief Enumerations for speech status.
* @since_tizen 3.0
*/
typedef enum {
	STTE_SPEECH_STATUS_BEGINNING_POINT_DETECTED = 0,	/**< Beginning point of speech is detected */
	STTE_SPEECH_STATUS_END_POINT_DETECTED		/**< End point of speech is detected */
} stte_speech_status_e;

/**
* @brief Definition for free form dictation and default type.
* @since_tizen 3.0
*/
#define STTE_RECOGNITION_TYPE_FREE		"stt.recognition.type.FREE"

/**
* @brief Definition for free form dictation continuously.
* @since_tizen 3.0
*/
#define STTE_RECOGNITION_TYPE_FREE_PARTIAL	"stt.recognition.type.FREE.PARTIAL"

/**
* @brief Definition for None message.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_NONE		"stt.result.message.none"

/**
* @brief Definition for failed recognition because the speech started too soon.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_ERROR_TOO_SOON	"stt.result.message.error.too.soon"

/**
* @brief Definition for failed recognition because the speech started too short.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_ERROR_TOO_SHORT	"stt.result.message.error.too.short"

/**
* @brief Definition for failed recognition because the speech started too long.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_ERROR_TOO_LONG	"stt.result.message.error.too.long"

/**
* @brief Definition for failed recognition because the speech started too quiet to listen.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_ERROR_TOO_QUIET	"stt.result.message.error.too.quiet"

/**
* @brief Definition for failed recognition because the speech started too loud to listen.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_ERROR_TOO_LOUD	"stt.result.message.error.too.loud"

/**
* @brief Definition for failed recognition because the speech started too fast to listen.
* @since_tizen 3.0
*/
#define STTE_RESULT_MESSAGE_ERROR_TOO_FAST	"stt.result.message.error.too.fast"


/**
* @brief Called when STT engine provides the time stamp of result to the engine service user.
* @details This callback function is implemented by the engine service user. Therefore, the engine developer does NOT have to implement this callback function.
* @since_tizen 3.0
*
* @remarks This callback function is called in stte_foreach_result_time_cb() for adding time information.
*	@a user_data must be transferred from stte_foreach_result_time_cb().
*
* @param[in] index The result index
* @param[in] event The token event
* @param[in] text The result text
* @param[in] start_time The time started speaking the result text
* @param[in] end_time The time finished speaking the result text
* @param[in] user_data The user data passed from stte_foreach_result_time_cb()
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre stte_send_result() should be called.
*
* @see stte_send_result()
* @see stte_foreach_result_time_cb()
*/
typedef bool (*stte_result_time_cb)(int index, stte_result_time_event_e event, const char* text,
				long start_time, long end_time, void* user_data);


/**
* @brief Called when STT engine informs the engine service user about whole supported language list.
* @details This callback function is implemented by the engine service user. Therefore, the engine developer does NOT have to implement this callback function.
* @since_tizen 3.0
*
* @remarks This callback function is called in stte_foreach_supported_langs_cb() to inform the whole supported language list.
*	@a user_data must be transferred from stte_foreach_supported_langs_cb().
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two letter country-code
*	followed by ISO 639-1 for the two-letter language code \n
*	For example, "ko_KR" for Korean, "en_US" for American English
* @param[in] user_data The user data passed from stte_foreach_supported_langs_cb()
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre stte_foreach_supported_langs_cb() will invoke this callback function.
*
* @see stte_foreach_supported_langs_cb()
*/
typedef bool (*stte_supported_language_cb)(const char* language, void* user_data);

/**
* @brief Called when the engine service user initializes STT engine.
* @details This callback function is called by the engine service user to request for STT engine to be started.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_INVALID_STATE Already initialized
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @see stte_deinitialize_cb()
*/
typedef int (*stte_initialize_cb)(void);

/**
* @brief Called when the engine service user deinitializes STT engine
* @details This callback function is called by the engine service user to request for STT engine to be deinitialized.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	NOTE that the engine may be terminated automatically.
*	When this callback function is invoked, the release of resources is necessary.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_STATE Not initialized
*
* @see stte_initialize_cb()
*/
typedef int (*stte_deinitialize_cb)(void);

/**
* @brief Called when the engine service user gets the whole supported language list.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	In this function, the engine service user's callback function 'stte_supported_language_cb()' is invoked repeatedly for getting all supported languages, and @a user_data must be transferred to 'stte_supported_language_cb()'.
*	If 'stte_supported_language_cb()' returns @c false, it should be stopped to call 'stte_supported_language_cb()'.
*
* @param[in] callback The callback function
* @param[in] user_data The user data which must be passed to the callback function 'stte_supported_language_cb()'
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_INVALID_STATE Not initialized
*
* @post	This callback function invokes stte_supported_language_cb() repeatedly for getting supported languages.
*
* @see stte_supported_language_cb()
*/
typedef int (*stte_foreach_supported_langs_cb)(stte_supported_language_cb callback, void* user_data);

/**
* @brief Called when the engine service user checks whether the corresponding language is valid or not in STT engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two letter country-code
*	followed by ISO 639-1 for the two-letter language code \n
*	For example, "ko_KR" for Korean, "en_US" for American English
* @param[out] is_valid A variable for checking whether the corresponding language is valid or not. \n @c true to be valid, @c false to be invalid.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see stte_foreach_supported_languages_cb()
*/
typedef int (*stte_is_valid_language_cb)(const char* language, bool* is_valid);

/**
* @brief Called when the engine service user checks whether STT engine supports silence detection.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @return @c true to support silence detection, @c false not to support silence detection
*
* @see stte_set_silence_detection_cb()
*/
typedef bool (*stte_support_silence_detection_cb)(void);

/**
* @brief Called when the engine service user checks whether STT engine supports the corresponding recognition type.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @param[in] type The type for recognition (e.g. #STTE_RECOGNITION_TYPE_FREE)
* @param[out] is_supported A variable for checking whether STT engine supports the corresponding recognition type. \n @c true to support recognition type, @c false not to support recognition type.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
*
*/
typedef int (*stte_support_recognition_type_cb)(const char* type, bool* is_supported);

/**
* @brief Called when the engine service user gets the proper recording format of STT engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	The recording format is used for creating the recorder.
*
* @param[out] types The format used by the recorder.
* @param[out] rate The sample rate used by the recorder.
* @param[out] channels The number of channels used by the recorder.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_STATE Not initialized
*/
typedef int (*stte_get_recording_format_cb)(stte_audio_type_e* types, int* rate, int* channels);

/**
* @brief Called when the engine service user sets the silence detection.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	If the engine service user sets this option as 'TRUE', STT engine will detect the silence (EPD) and send the callback event about it.
*
* @param[in] is_set A variable for setting the silence detection. \n @c true to detect the silence, @c false not to detect the silence.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_STATE Not initialized
* @retval #STTE_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*/
typedef int (*stte_set_silence_detection_cb)(bool is_set);

/**
* @brief Called when the engine service user requests for STT engine to check whether the application agreed the usage of STT engine.
* @details This callback function is called when the engine service user requests for STT engine to check the application's agreement about using the engine.
*	According to the need, the engine developer can provide some user interfaces to check the agreement.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	If the STT engine developer wants not to check the agreement, the developer has need to return proper values as @a is_agreed in accordance with the intention. \n @c true if the developer regards that every application agreed the usage of the engine, @c false if the developer regards that every application disagreed.
*	NOTE that, however, there may be any legal issue unless the developer checks the agreement. Therefore, we suggest that the engine developers should provide a function to check the agreement.
*
* @param[in] appid The Application ID
* @param[out] is_agreed A variable for checking whether the application agreed to use STT engine or not. \n @c true to agree, @c false to disagree.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_STATE Not initialized
* @retval #STTE_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*/
typedef int (*stte_check_app_agreed_cb)(const char* appid, bool* is_agreed);

/**
* @brief Called when the engine service user checks whether STT engine needs the application's credential.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @return @c true if STT engine needs the application's credential, otherwise @c false
*/
typedef bool (*stte_need_app_credential_cb)(void);

/**
* @brief Called when the engine service user gets the result time information(stamp).
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	In this function, the engine service user's callback function 'stte_result_time_cb()' is invoked repeatedly for sending the time information to the engine service user, and @a user_data must be transferred to 'stte_result_time_cb()'.
*	If 'stte_result_time_cb()' returns @c false, it should be stopped to call 'stte_result_time_cb()'.
*	@a time_info is transferred from stte_send_result(). The type of @a time_info is up to the STT engine developer.
*
* @param[in] time_info The time information
* @param[in] callback The callback function
* @param[in] user_data The user data which must be passed to the callback function 'stte_result_time_cb()'
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_INVALID_STATE Not initialized
*
* @pre stte_send_result() will invoke this function.
* @post	This function invokes stte_result_time_cb() repeatedly for getting result time information.
*
* @see stte_result_time_cb()
*/
typedef int (*stte_foreach_result_time_cb)(void* time_info, stte_result_time_cb callback, void* user_data);

/**
* @brief Called when the engine service user starts to recognize the recording data.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	In this callback function, STT engine must transfer the recognition result and @a user_data to the engine service user using stte_send_result().
*	Also, if STT engine needs the application's credential, it sets the credential granted to the application.
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two letter country-code
*	followed by ISO 639-1 for the two-letter language code \n
*	For example, "ko_KR" for Korean, "en_US" for American English
* @param[in] type The recognition type. (e.g. #STTE_RECOGNITION_TYPE_FREE)
* @param[in] appid The Application ID
* @param[in] credential The credential granted to the application
* @param[in] user_data The user data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_INVALID_STATE Invalid state
* @retval #STTE_ERROR_INVALID_LANGUAGE Invalid language
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
* @retval #STTE_ERROR_NETWORK_DOWN Out of network
*
* @pre The engine is not in recognition processing.
*
* @see stte_set_recording_data_cb()
* @see stte_stop_cb()
* @see stte_cancel_cb()
* @see stte_need_app_credential_cb()
*/
typedef int (*stte_start_cb)(const char* language, const char* type, const char* appid, const char* credential, void *user_data);

/**
* @brief Called when the engine service user sets and sends the recording data for speech recognition.
* @details This callback function is called by the engine service user to send the recording data to STT engine.
*	The engine receives the recording data and uses for speech recognition.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	Also, this function should be returned immediately after recording data copy.
*
* @param[in] data The recording data
* @param[in] length The length of recording data
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_INVALID_STATE Invalid state
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @pre stte_start_cb() should succeed.
* @post If the engine supports partial result, stte_send_result() should be invoked.
*
* @see stte_start_cb()
* @see stte_cancel_cb()
* @see stte_stop_cb()
*/
typedef int (*stte_set_recording_data_cb)(const void* data, unsigned int length);

/**
* @brief Called when the engine service user stops to recognize the recording data.
* @details This callback function is called by the engine service user to stop recording and to get the recognition result.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_STATE Invalid state
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
* @retval #STTE_ERROR_NETWORK_DOWN Out of network
*
* @pre stte_start_cb() should succeed.
* @post After processing of the engine, stte_send_result() must be called.
*
* @see stte_start_cb()
* @see stte_set_recording_data_cb()
* @see stte_cancel_cb()
* @see stte_send_result()
*/
typedef int (*stte_stop_cb)(void);

/**
* @brief Called when the engine service user cancels to recognize the recording data.
* @details This callback function is called by the engine service user to cancel to recognize the recording data.
*	Also, when starting the recorder is failed, this function is called.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*
* @return 0 on success, otherwise a negative error value.
* @retval #STTE_ERROR_NONE Successful.
* @retval #STTE_ERROR_INVALID_STATE Invalid state.
*
* @pre STT engine is in recognition processing or recording.
*
* @see stte_start_cb()
* @see stte_stop_cb()
*/
typedef int (*stte_cancel_cb)(void);

/**
* @brief Called when the engine service user requests the basic information of STT engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using stte_main().
*	The allocated @a engine_uuid, @a engine_name, and @a engine_setting will be released internally.
*	In order to upload the engine at Tizen Appstore, both a service app and a ui app are necessary.
*	Therefore, @a engine_setting must be transferred to the engine service user.
*
* @param[out] engine_uuid UUID of engine
* @param[out] engine_name Name of engine
* @param[out] engine_setting The engine setting application(ui app)'s app id
* @param[out] use_network A variable for checking whether the network is used or not
*
* @return 0 on success, otherwise a negative error code on failure
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
*/
typedef int (*stte_get_info_cb)(char** engine_uuid, char** engine_name, char** engine_setting, bool* use_network);

/**
* @brief Called when STT engine receives the private data from the engine service user.
* @details This callback function is called when the engine service user sends the private data to STT engine.
* @since_tizen 3.0
*
* @remarks This callback function is optional and is registered using stte_set_private_data_set_cb().
*
* @param[in] key The key field of private data.
* @param[in] data The data field of private data.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @see stte_private_data_requested_cb()
* @see stte_set_private_data_set_cb()
*/
typedef int (*stte_private_data_set_cb)(const char* key, const char* data);

/**
* @brief Called when STT engine provides the engine service user with the private data.
* @details This callback function is called when the engine service user gets the private data from STT engine.
* @since_tizen 3.0
*
* @remarks This callback function is optional and is registered using stte_set_private_data_requested_cb().
*
* @param[out] key The key field of private data.
* @param[out] data The data field of private data.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @see stte_private_data_set_cb()
* @see stte_set_private_data_requested_cb()
*/
typedef int (*stte_private_data_requested_cb)(const char* key, char** data);

/**
* @brief A structure for the STT engine functions.
* @details This structure contains essential callback functions for operating STT engine.
* @since_tizen 3.0
*
* @remarks These functions are mandatory for operating STT engine. Therefore, all functions MUST be implemented.
*/
typedef struct {
	int version;								/**< The version of the structure 'stte_request_callback_s' */
	stte_get_info_cb			get_info;			/**< Called when the engine service user requests the basic information of STT engine */

	stte_initialize_cb			initialize;			/**< Called when the engine service user initializes STT engine */
	stte_deinitialize_cb			deinitialize;			/**< Called when the engine service user deinitializes STT engine */

	stte_foreach_supported_langs_cb		foreach_langs;			/**< Called when the engine service user gets the whole supported language list */
	stte_is_valid_language_cb		is_valid_lang;			/**< Called when the engine service user checks whether the corresponding language is valid or not*/
	stte_support_silence_detection_cb	support_silence;		/**< Called when the engine service user checks whether STT engine supports silence detection*/
	stte_support_recognition_type_cb	support_recognition_type;	/**< Called when the engine service user checks whether STT engine supports the corresponding recognition type */
	stte_get_recording_format_cb		get_audio_format;		/**< Called when the engine service user gets the proper recording format of STT engine */
	stte_foreach_result_time_cb		foreach_result_time;		/**< Called when the engine service user gets the result time information(stamp) */

	stte_set_silence_detection_cb		set_silence_detection;		/**< Called when the engine service user sets the silence detection */

	stte_start_cb				start;				/**< Called when the engine service user starts to recognize the recording data */
	stte_set_recording_data_cb		set_recording;			/**< Called when the engine service user sets and sends the recording data for speech recognition */
	stte_stop_cb				stop;				/**< Called when the engine service user stops to recognize the recording data */
	stte_cancel_cb				cancel;				/**< Called when the engine service user cancels to recognize the recording data */

	stte_check_app_agreed_cb		check_app_agreed;		/**< Called when the engine service user requests for STT engine to check whether the application agreed the usage of STT engine */
	stte_need_app_credential_cb		need_app_credential;		/**< Called when the engine service user checks whether STT engine needs the application's credential */
} stte_request_callback_s;

/**
* @brief Main function for Speech-To-Text (STT) engine.
* @details This function is the main function for operating STT engine.
* @since_tizen 3.0
*
* @privlevel public
* @privilege %http://tizen.org/privilege/recorder
*
* @remarks The service_app_main() should be used for working the engine after this function.
*
* @param[in] argc The argument count(original)
* @param[in] argv The argument(original)
* @param[in] callback The structure of engine request callback function
*
* @return This function returns zero on success, or negative with error code on failure
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_PERMISSION_DENIED	Permission denied
* @retval #STTE_ERROR_NOT_SUPPORTED Not supported
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @see stte_request_callback_s
*
* @code
#include <stte.h>

// Required callback functions - MUST BE IMPLEMENTED
static int sttengine_get_info_cb(char** engine_uuid, char** engine_name, char** engine_setting, bool* use_network);
static int sttengine_initialize_cb(void);
static int sttengine_deinitialize_cb(void);
static int sttengine_is_valid_language_cb(const char* language, bool* is_valid);
static int sttengine_foreach_supported_langs_cb(stte_supported_language_cb callback, void* user_data);
static bool sttengine_support_silence_detection_cb(void);
static int sttengine_set_silence_detection_cb(bool is_set);
static int sttengine_support_recognition_type_cb(const char* type, bool* is_supported);
static int sttengine_get_recording_format_cb(stte_audio_type_e* types, int* rate, int* channels);
static int sttengine_set_recording_data_cb(const void* data, unsigned int length);
static int sttengine_foreach_result_time_cb(void* time_info, stte_result_time_cb callback, void* user_data);
static int sttengine_start_cb(const char* language, const char* type, const char* appid, const char* credential, void *user_data);
static int sttengine_stop_cb(void);
static int sttengine_cancel_cb(void);
static int sttengine_check_app_agreed_cb(const char* appid, bool* is_agreed);
static bool sttengine_need_app_credential_cb(void);

// Optional callback function
static int sttengine_private_data_set_cb(const char* key, const char* data);

int main(int argc, char* argv[])
{
	// 1. Create a structure 'stte_request_callback_s'
	stte_request_callback_s engine_callback = { 0, };

	engine_callback.size = sizeof(stte_request_callback_s);
	engine_callback.version = 1;
	engine_callback.get_info = sttengine_get_info_cb;

	engine_callback.initialize = sttengine_initialize_cb;
	engine_callback.deinitialize = sttengine_deinitialize_cb;

	engine_callback.foreach_langs = sttengine_foreach_supported_langs_cb;
	engine_callback.is_valid_lang = sttengine_is_valid_language_cb;
	engine_callback.support_silence = sttengine_support_silence_detection_cb;
	engine_callback.support_recognition_type = sttengine_support_recognition_type_cb;

	engine_callback.get_audio_format = sttengine_get_recording_format_cb;
	engine_callback.foreach_result_time = sttengine_foreach_result_time_cb;

	engine_callback.set_silence_detection = sttengine_set_silence_detection_cb;

	engine_callback.start = sttengine_start_cb;
	engine_callback.set_recording = sttengine_set_recording_data_cb;
	engine_callback.stop = sttengine_stop_cb;
	engine_callback.cancel = sttengine_cancel_cb;

	engine_callback.check_app_agreed = sttengine_check_app_agreed_cb;
	engine_callback.need_app_credential = sttengine_need_app_credential_cb;

	// 2. Run 'stte_main()'
	if (0 != stte_main(argc, argv, &engine_callback)) {
		return -1;
	}

	// Optional
	stte_set_private_data_set_cb(sttengine_private_data_set_cb);

	// 3. Set event callbacks for service app and Run 'service_app_main()'
	char ad[50] = { 0, };

	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	return service_app_main(argc, argv, &event_callback, ad);
}

* @endcode
*/
int stte_main(int argc, char** argv, stte_request_callback_s *callback);


/**
* @brief Sends the recognition result to the engine service user.
* @since_tizen 3.0
*
* @remarks This API is used in stte_set_recording_data_cb() and stte_stop_cb(), when STT engine sends the recognition result to the engine service user.
*	This function is called in the following situations; 1) after stte_stop_cb() is called, 2) the end point of speech is detected from recording, or 3) partial result is occurred.
*	The recognition result and @a user_data must be transferred to the engine service user through this function.
*	Also, @a time_info must be transferred to stte_foreach_result_time_cb(). The type of @a time_info is up to the STT engine developer.
*
* @param[in] event The result event
* @param[in] type The recognition type (e.g. #STTE_RECOGNITION_TYPE_FREE, #STTE_RECOGNITION_TYPE_FREE_PARTIAL)
* @param[in] result Result texts
* @param[in] result_count Result text count
* @param[in] msg Engine message (e.g. #STTE_RESULT_MESSAGE_NONE, #STTE_RESULT_MESSAGE_ERROR_TOO_SHORT)
* @param[in] time_info The time information
* @param[in] user_data	The user data passed from stte_start_cb()
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_PERMISSION_DENIED	Permission denied
* @retval #STTE_ERROR_NOT_SUPPORTED Not supported
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The stte_main() function should be invoked before this function is called.
*	stte_stop_cb() will invoke this function.
* @post This function invokes stte_foreach_result_time_cb().
*
* @see stte_start_cb()
* @see stte_set_recording_data_cb()
* @see stte_stop_cb()
* @see stte_foreach_result_time_cb()
*/
int stte_send_result(stte_result_event_e event, const char* type, const char** result, int result_count,
				const char* msg, void* time_info, void* user_data);


/**
* @brief Sends the error to the engine service user.
* @details The following error codes can be delivered.\n
*	#STTE_ERROR_NONE,\n
*	#STTE_ERROR_OUT_OF_MEMORY,\n
*	#STTE_ERROR_IO_ERROR,\n
*	#STTE_ERROR_INVALID_PARAMETER,\n
*	#STTE_ERROR_NETWORK_DOWN,\n
*	#STTE_ERROR_INVALID_STATE,\n
*	#STTE_ERROR_INVALID_LANGUAGE,\n
*	#STTE_ERROR_OPERATION_FAILED,\n
*	#STTE_ERROR_NOT_SUPPORTED_FEATURE,\n
*	#STTE_ERROR_NOT_SUPPORTED,\n
*	#STTE_ERROR_PERMISSION_DENIED,\n
*	#STTE_ERROR_RECORDING_TIMED_OUT.\n
*
* @since_tizen 3.0
*
* @param[in] error The error reason
* @param[in] msg The error message
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_PERMISSION_DENIED	Permission denied
* @retval #STTE_ERROR_NOT_SUPPORTED Not supported
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The stte_main() function should be invoked before this function is called.
*/
int stte_send_error(stte_error_e error, const char* msg);

/**
* @brief Sends the speech status to the engine service user when STT engine notifies the change of the speech status.
* @since_tizen 3.0
*
* @remarks This API is invoked when STT engine wants to notify the change of the speech status anytime.
*	NOTE that this API can be invoked for recognizing the speech.
*
* @param[in] status The status of speech (e.g. STTE_SPEECH_STATUS_START_POINT_DETECTED or STTE_SPEECH_STATUS_END_POINT_DETECTED)
* @param[in] user_data The user data passed from the start function.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_PERMISSION_DENIED	Permission denied
* @retval #STTE_ERROR_NOT_SUPPORTED Not supported
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The stte_main() function should be invoked before this function is called.
*	stte_start_cb() and stte_set_recording_data_cb() will invoke this function.
*
* @see stte_start_cb()
* @see stte_set_recording_data_cb()
*/
int stte_send_speech_status(stte_speech_status_e status, void* user_data);


/**
* @brief Sets a callback function for setting the private data.
* @since_tizen 3.0
*
* @privlevel public
* @privilege %http://tizen.org/privilege/recorder
*
* @remarks The stte_private_data_set_cb() function is called when the engine service user sends the private data.
*
* @param[in] callback_func stte_private_data_set event callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_PERMISSION_DENIED	Permission denied
* @retval #STTE_ERROR_NOT_SUPPORTED Not supported
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The stte_main() function should be invoked before this function is called.
*
* @see stte_private_data_set_cb()
*/
int stte_set_private_data_set_cb(stte_private_data_set_cb callback_func);

/**
* @brief Sets a callback function for requesting the private data.
* @since_tizen 3.0
*
* @privlevel public
* @privilege %http://tizen.org/privilege/recorder
*
* @remarks The stte_private_data_requested_cb() function is called when the engine service user gets the private data from STT engine.
*
* @param[in] callback_func stte_private_data_requested event callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STTE_ERROR_NONE Successful
* @retval #STTE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTE_ERROR_PERMISSION_DENIED	Permission denied
* @retval #STTE_ERROR_NOT_SUPPORTED Not supported
* @retval #STTE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The stte_main() function should be invoked before this function is called.
*
* @see stte_private_data_requested_cb()
*/
int stte_set_private_data_requested_cb(stte_private_data_requested_cb callback_func);



#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __STT_ENGINE_MAIN_H__ */
