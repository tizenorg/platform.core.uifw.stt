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

#ifndef __STTP_H__
#define __STTP_H__

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup STT_ENGINE_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/** 
* @brief Enumerations of error codes.
*/
typedef enum {
	STTP_ERROR_NONE			=  0,		/**< Successful */
	STTP_ERROR_OUT_OF_MEMORY	= -ENOMEM,	/**< Out of Memory */
	STTP_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	STTP_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	STTP_ERROR_OUT_OF_NETWORK	= -ENETDOWN,	/**< Out of network */
	STTP_ERROR_INVALID_STATE	= -0x0100031,	/**< Invalid state */
	STTP_ERROR_INVALID_LANGUAGE	= -0x0100032,	/**< Invalid language */
	STTP_ERROR_OPERATION_FAILED	= -0x0100034,	/**< Operation failed */
	STTP_ERROR_NOT_SUPPORTED_FEATURE= -0x0100035	/**< Not supported feature */
}sttp_error_e;

/**
* @brief Enumerations of audio type. 
*/
typedef enum {
	STTP_AUDIO_TYPE_PCM_S16_LE = 0,	/**< Signed 16bit audio type, Little endian */
	STTP_AUDIO_TYPE_PCM_U8		/**< Unsigned 8bit audio type */
}sttp_audio_type_e;

/**
* @brief Enumerations of callback event.
*/
typedef enum {
	STTP_RESULT_EVENT_FINAL_RESULT = 0,	/**< Event when the recognition full or last result is ready  */
	STTP_RESULT_EVENT_PARTIAL_RESULT,	/**< Event when the recognition partial result is ready  */
	STTP_RESULT_EVENT_ERROR			/**< Event when the recognition has failed */
}sttp_result_event_e;

/**
* @brief Enumerations of result time callback event.
*/
typedef enum {
	STTP_RESULT_TIME_EVENT_BEGINNING = 0,	/**< Event when the token is beginning type */
	STTP_RESULT_TIME_EVENT_MIDDLE,		/**< Event when the token is middle type */
	STTP_RESULT_TIME_EVENT_END		/**< Event when the token is end type */
}sttp_result_time_event_e;

/**
* @brief Enumerations of silence type.
*/
typedef enum {
	STTP_SILENCE_TYPE_NO_RECORD_TIMEOUT = 0,	/**< No sound is recorded */
	STTP_SILENCE_TYPE_END_OF_SPEECH_DETECTED	/**< End of speech is detected */
}sttp_silence_type_e;

/** 
* @brief Recognition type : free form dictation and default type.
*/
#define STTP_RECOGNITION_TYPE_FREE		"stt.recognition.type.FREE"

/** 
* @brief Recognition type : free form dictation continuously. 
*/
#define STTP_RECOGNITION_TYPE_FREE_PARTIAL	"stt.recognition.type.FREE.PARTIAL"

/**
* @brief Recognition type : Search. 
*/
#define STTP_RECOGNITION_TYPE_SEARCH		"stt.recognition.type.SEARCH"

/** 
* @brief Recognition type : web search. 
*/
#define STTP_RECOGNITION_TYPE_WEB_SEARCH	"stt.recognition.type.WEB_SEARCH"

/**
* @brief Recognition type : Map. 
*/
#define STTP_RECOGNITION_TYPE_MAP		"stt.recognition.type.MAP"


/** 
* @brief Result message : None message
*/
#define STTP_RESULT_MESSAGE_NONE		"stt.result.message.none"

/** 
* @brief Result error message : Recognition was failed because the speech started too soon
*/
#define STTP_RESULT_MESSAGE_ERROR_TOO_SOON	"stt.result.message.error.too.soon"

/** 
* @brief Result error message : Recognition was failed because the speech started too short
*/
#define STTP_RESULT_MESSAGE_ERROR_TOO_SHORT	"stt.result.message.error.too.short"

/** 
* @brief Result error message : Recognition was failed because the speech started too long
*/
#define STTP_RESULT_MESSAGE_ERROR_TOO_LONG	"stt.result.message.error.too.long"

/** 
* @brief Result error message : Recognition was failed because the speech started too quiet to listen
*/
#define STTP_RESULT_MESSAGE_ERROR_TOO_QUIET	"stt.result.message.error.too.quiet"

/** 
* @brief Result error message : Recognition was failed because the speech started too loud to listen 
*/
#define STTP_RESULT_MESSAGE_ERROR_TOO_LOUD	"stt.result.message.error.too.loud"

/** 
* @brief Result error message : Recognition was failed because the speech started too fast to listen
*/
#define STTP_RESULT_MESSAGE_ERROR_TOO_FAST	"stt.result.message.error.too.fast"

/** 
* @brief Called to get recognition result.
* 
* @param[in] event A result event
* @param[in] type A recognition type (e.g. #STTP_RECOGNITION_TYPE_FREE, #STTP_RECOGNITION_TYPE_FREE_PARTIAL)
* @param[in] data Result texts
* @param[in] data_count Result text count
* @param[in] msg Engine message (e.g. #STTP_RESULT_MESSAGE_NONE, #STTP_RESULT_MESSAGE_ERROR_TOO_SHORT)
* @param[in] user_data	The user data passed from the start function
*
* @pre sttpe_stop() will invoke this callback.
*
* @see sttpe_start()
* @see sttpe_stop()
*/
typedef void (*sttpe_result_cb)(sttp_result_event_e event, const char* type, const char** data, int data_count, 
				const char* msg, void* time_info, void* user_data);

/**
* @brief Called to retrieve the time stamp of result. 
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
* @pre sttpe_result_cb() should be called. 
*
* @see sttpe_result_cb()
*/
typedef bool (*sttpe_result_time_cb)(int index, sttp_result_time_event_e event, const char* text, 
				long start_time, long end_time, void* user_data);

/** 
* @brief Called to detect silence from recording data.
* 
* @param[in] user_data	The user data passed from the start function.
*
* @pre sttpe_set_recording_data() will invoke this callback.
*
* @see sttpe_set_recording_data()
*/
typedef void (*sttpe_silence_detected_cb)(sttp_silence_type_e type, void* user_data);

/**
* @brief Called to retrieve the supported languages. 
*
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code
*		followed by ISO 639-1 for the two-letter language code \n
*		For example, "ko_KR" for Korean, "en_US" for American English
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre sttpe_foreach_supported_languages() will invoke this callback. 
*
* @see sttpe_foreach_supported_languages()
*/
typedef bool (*sttpe_supported_language_cb)(const char* language, void* user_data);

/**
* @brief Initializes the engine.
*
* @param[in] result_cb A callback function for recognition result
* @param[in] silence_cb A callback function for silence detection
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_INVALID_STATE Already initialized
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
* 
* @see sttpe_deinitialize()
*/
typedef int (* sttpe_initialize)(sttpe_result_cb result_cb, sttpe_silence_detected_cb silence_cb);

/**
* @brief Deinitializes the engine
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_STATE Not initialized
* 
* @see sttpe_initialize()
*/
typedef int (* sttpe_deinitialize)(void);

/**
* @brief Retrieves all supported languages of the engine.
*
* @param[in] callback a callback function
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_INVALID_STATE Not initialized
*
* @post	This function invokes sttpe_supported_language_cb() repeatedly for getting supported languages. 
*
* @see sttpe_supported_language_cb()
*/
typedef int (* sttpe_foreach_supported_langs)(sttpe_supported_language_cb callback, void* user_data);

/**
* @brief Checks whether a language is valid or not.
*
* @param[in] language A language
*
* @return @c true to valid language, \n @c false to invalid language.
*
* @see sttpe_foreach_supported_languages()
*/
typedef bool (* sttpe_is_valid_language)(const char* language);

/**
* @brief Gets whether the engine supports silence detection. 
*
* @return @c true to support silence detection, \n @c false not to support silence detection.
*
* @see sttpe_set_silence_detection()
*/
typedef bool (* sttpe_support_silence_detection)(void);

/**
* @brief Gets supporting recognition type. 
*
* @return @c true to support recognition type, \n @c false not to support recognition type.
*
*/
typedef bool (* sttpe_support_recognition_type)(const char* type);

/**
* @brief Gets recording format of the engine. 
*
* @param[out] types The format used by the recorder.
* @param[out] rate The sample rate used by the recorder.
* @param[out] channels The number of channels used by the recorder.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_STATE Not initialized
*/
typedef int (* sttpe_get_recording_format)(sttp_audio_type_e* types, int* rate, int* channels);

/**
* @brief Sets silence detection option.
* 
* @param[in] value A value
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_STATE Not initialized
* @retval #STTP_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*/
typedef int (* sttpe_set_silence_detection)(bool value);

/**
* @brief Gets whether application is agreed to get engine service.
* 
* @param[in] appid Application ID
* @param[in] value A value
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_STATE Not initialized
* @retval #STTP_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*/
typedef int (* sttpe_check_app_agreed)(const char* appid, bool* value);

/**
* @brief Retrieves result time info in recognition callback of daemon.
*
* @param[in] callback a callback function
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_INVALID_STATE Not initialized
*
* @pre sttpe_result_cb() will be invoke this function.
* @post	This function invokes sttpe_result_time_cb() repeatedly for getting result time information. 
*
* @see sttpe_result_time_cb()
*/
typedef int (* sttpe_foreach_result_time)(void* time_info, sttpe_result_time_cb callback, void* user_data);

/**
* @brief Start recognition.
*
* @param[in] language A language. 
* @param[in] type A recognition type. (e.g. #STTP_RECOGNITION_TYPE_FREE, #STTP_RECOGNITION_TYPE_WEB_SEARCH)
* @param[in] user_data The user data to be passed to the callback function. 
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_INVALID_STATE Invalid state
* @retval #STTP_ERROR_INVALID_LANGUAGE Invalid language
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
* @retval #STTP_ERROR_OUT_OF_NETWORK Out of network
*
* @pre The engine is not in recognition processing.
*
* @see sttpe_set_recording_data()
* @see sttpe_stop()
* @see sttpe_cancel()
*/
typedef int (* sttpe_start)(const char* language, const char* type, void *user_data);

/**
* @brief Sets recording data for speech recognition from recorder. 
*
* @remark This function should be returned immediately after recording data copy. 
* 
* @param[in] data A recording data
* @param[in] length A length of recording data
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_INVALID_STATE Invalid state
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
*
* @pre sttpe_start() should succeed.
* @post If the engine supports partial result, it will invoke sttpe_result_cb().
*
* @see sttpe_start()
* @see sttpe_cancel()
* @see sttpe_stop()
*/
typedef int (* sttpe_set_recording_data)(const void* data, unsigned int length);

/**
* @brief Stops to set recording data.
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_STATE Invalid state
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
* @retval #STTP_ERROR_OUT_OF_NETWORK Out of network
*
* @pre sttpe_start() should succeed.
* @post After processing of the engine, sttpe_result_cb() is called.
*
* @see sttpe_start()
* @see sttpe_set_recording_data()
* @see sttpe_result_cb()
* @see sttpe_cancel()
*/
typedef int (* sttpe_stop)(void);

/**
* @brief Cancels the recognition process.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STTP_ERROR_NONE Successful.
* @retval #STTP_ERROR_INVALID_STATE Invalid state.
* @pre STT engine is in recognition processing or recording.
*
* @see sttpe_start()
* @see sttpe_stop()
*/
typedef int (* sttpe_cancel)(void);

/**
* @brief Start recognition of file.
*
* @param[in] language A language. 
* @param[in] type A recognition type. (e.g. #STTP_RECOGNITION_TYPE_FREE, #STTP_RECOGNITION_TYPE_WEB_SEARCH)
* @param[in] filepath A filepath for recognition.
* @param[in] audio_type A audio type of file.
* @param[in] sample_rate A sample rate of file.
* @param[in] user_data The user data to be passed to the callback function. 
*
* @return 0 on success, otherwise a negative error value
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_INVALID_STATE Invalid state
* @retval #STTP_ERROR_INVALID_LANGUAGE Invalid language
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
* @retval #STTP_ERROR_OUT_OF_NETWORK Out of network
*
* @pre The engine is not in recognition processing.
*
* @see sttpe_cancel_file()
*/
typedef int (* sttpe_start_file)(const char* language, const char* type, const char* filepath, 
				 sttp_audio_type_e audio_type, int sample_rate, void *user_data);

/**
* @brief Cancels the recognition process of file.
*
* @return 0 on success, otherwise a negative error value.
* @retval #STTP_ERROR_NONE Successful.
* @retval #STTP_ERROR_INVALID_STATE Invalid state.
* @pre STT engine is in recognition processing.
*
* @see sttpe_start_file()
*/
typedef int (* sttpe_cancel_file)(void);

/**
* @brief A structure of the engine functions.
*/
typedef struct {
	int size;						/**< Size of structure */    
	int version;						/**< Version */
	
	sttpe_initialize		initialize;		/**< Initialize engine */
	sttpe_deinitialize		deinitialize;		/**< Shutdown engine */

	/* Get engine information */
	sttpe_foreach_supported_langs	foreach_langs;		/**< Foreach language list */
	sttpe_is_valid_language		is_valid_lang;		/**< Check language */
	sttpe_support_silence_detection	support_silence;	/**< Get silence detection support */
	sttpe_support_recognition_type	support_recognition_type; /**< Get recognition type support */
	sttpe_get_recording_format	get_audio_format;	/**< Get audio format */
	sttpe_foreach_result_time	foreach_result_time;	/**< Foreach result time */
	
	/* Set engine information */
	sttpe_set_silence_detection	set_silence_detection;	/**< Set silence detection */

	/* Check app agreement */
	sttpe_check_app_agreed		check_app_agreed;	/**< Get app agreement */

	/* Control recognition */
	sttpe_start			start;			/**< Start recognition */
	sttpe_set_recording_data	set_recording;		/**< Set recording data */
	sttpe_stop			stop;			/**< Shutdown function */
	sttpe_cancel			cancel;			/**< Cancel recognition */

	/* Control file recognition */
	sttpe_start_file		start_file;		/**< Start recognition */
	sttpe_cancel_file		cancel_file;		/**< Cancel recognition */
} sttpe_funcs_s;

/**
* @brief A structure of the daemon functions.
*/
typedef struct {
	int size;						/**< size */
	int version;						/**< version */

} sttpd_funcs_s;

/**
* @brief Loads the engine. 
*
* @param[in] pdfuncs The daemon functions
* @param[out] pefuncs The engine functions
*
* @return This function returns zero on success, or negative with error code on failure
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
*
* @pre The sttp_get_engine_info() should be successful.
* @post The daemon calls engine functions of sttpe_funcs_s.
*
* @see sttp_get_engine_info()
* @see sttp_unload_engine()
*/
int sttp_load_engine(sttpd_funcs_s* pdfuncs, sttpe_funcs_s* pefuncs);

/**
* @brief Unloads this engine by the daemon. 
*
* @pre The sttp_load_engine() should be successful.
*
* @see sttp_load_engine()
*/
void sttp_unload_engine(void);

/**
* @brief Called to get the engine base information.
*
* @param[in] engine_uuid The engine id
* @param[in] engine_name The engine name
* @param[in] engine_setting The setting name
* @param[in] use_network @c true to need network @c false not to need network.
* @param[in] user_data The User data passed from sttp_get_engine_info()
*
* @pre sttp_get_engine_info() will invoke this callback. 
*
* @see sttp_get_engine_info()
*/
typedef void (*sttpe_engine_info_cb)(const char* engine_uuid, const char* engine_name, const char* engine_setting, 
				     bool use_network, void* user_data);

/**
* @brief Gets the engine base information before the engine is loaded by the daemon. 
*
* @param[in] callback Callback function
* @param[in] user_data User data to be passed to the callback function
*
* @return This function returns zero on success, or negative with error code on failure
* @retval #STTP_ERROR_NONE Successful
* @retval #STTP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STTP_ERROR_OPERATION_FAILED Operation failed
*
* @post	This function invokes sttpe_engine_info_cb() for getting engine information.
*
* @see sttpe_engine_info_cb()
* @see sttp_load_engine()
*/
int sttp_get_engine_info(sttpe_engine_info_cb callback, void* user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */
 
#endif /* __STTP_H__ */
