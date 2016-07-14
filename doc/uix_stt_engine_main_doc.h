/*
 * Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
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


#ifndef __TIZEN_UIX_STT_ENGINE_DOC_H__
#define __TIZEN_UIX_STT_ENGINE_DOC_H__

/**
 * @ingroup CAPI_UIX_FRAMEWORK
 * @defgroup CAPI_UIX_STTE_MODULE STT Engine
 * @brief The @ref CAPI_UIX_STTE_MODULE APIs provide functions to operate Speech-To-Text Engine.
 *
 *
 * @section CAPI_UIX_STTE_MODULE_HEADER Required Header
 *	 \#include <stte.h>
 *
 *
 * @section CAPI_UIX_STTE_MODULE_OVERVIEW Overview
 * Speech-To-Text Engine (below STTE) is an engine for recording speech and resulting in texts of speech recognition.
 * Using the @ref CAPI_UIX_STTE_MODULE APIs, STTE developers can provide STTE service users, who want to apply STTE, with functions necessary to operate the engine.
 * According to the indispensability of STTE services, there are two ways to provide them to the users. <br>
 *
 * <b>A. Required STTE services</b> <br>
 * These services are indispensable to operate STTE. Therefore, the STTE developers MUST implement callback functions corresponding to the required STTE services.
 * The following is a list of the callback functions. <br>
 *
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>stte_get_info_cb()</td>
 * <td>Called when the engine service user requests the basic information of STT engine.</td>
 * </tr>
 * <tr>
 * <td>stte_initialize_cb()</td>
 * <td>Called when the engine service user initializes STT engine.</td>
 * </tr>
 * <tr>
 * <td>stte_deinitialize_cb()</td>
 * <td>Called when the engine service user deinitializes STT engine.</td>
 * </tr>
 * <tr>
 * <td>stte_is_valid_language_cb()</td>
 * <td>Called when the engine service user checks whether the corresponding language is valid or not.</td>
 * </tr>
 * <tr>
 * <td>stte_foreach_supported_langs_cb()</td>
 * <td>Called when the engine service user gets the whole supported language list.</td>
 * </tr>
 * <tr>
 * <td>stte_support_silence_detection_cb()</td>
 * <td>Called when the engine service user checks whether STT engine supports silence detection.</td>
 * </tr>
 * <tr>
 * <td>stte_set_silence_detection_cb()</td>
 * <td>Called when the engine service user sets the silence detection.</td>
 * </tr>
 * <tr>
 * <td>stte_support_recognition_type_cb()</td>
 * <td>Called when the engine service user checks whether STT engine supports the corresponding recognition type.</td>
 * </tr>
 * <tr>
 * <td>stte_get_recording_format_cb()</td>
 * <td>Called when the engine service user gets the proper recording format of STT engine.</td>
 * </tr>
 * <tr>
 * <td>stte_set_recording_data_cb()</td>
 * <td>Called when the engine service user sets and sends the recording data for speech recognition.</td>
 * </tr>
 * <tr>
 * <td>stte_foreach_result_time_cb()</td>
 * <td>Called when the engine service user gets the result time information(stamp).</td>
 * </tr>
 * <tr>
 * <td>stte_start_cb()</td>
 * <td>Called when the engine service user starts to recognize the recording data.</td>
 * </tr>
 * <tr>
 * <td>stte_stop_cb()</td>
 * <td>Called when the engine service user stops to recognize the recording data.</td>
 * </tr>
 * <tr>
 * <td>stte_cancel_cb()</td>
 * <td>Called when the engine service user cancels to recognize the recording data.</td>
 * </tr>
 * <tr>
 * <td>stte_check_app_agreed_cb()</td>
 * <td>Called when the engine service user requests for STT engine to check whether the application agreed the usage of STT engine.</td>
 * </tr>
 * <tr>
 * <td>stte_need_app_credential_cb()</td>
 * <td>Called when the engine service user checks whether STT engine needs the application's credential.</td>
 * </tr>
 * </table>
 *
 * The STTE developers can register the above callback functions at a time with using a structure 'stte_request_callback_s' and an API 'stte_main()'.
 * To operate STTE, the following steps should be used: <br>
 * 1. Create a structure 'stte_request_callback_s'. <br>
 * 2. Implement callback functions. (NOTE that the callback functions should return appropriate values in accordance with the instruction.
 *	If the callback function returns an unstated value, STT framework will handle it as #STTE_ERROR_OPERATION_FAILED.)<br>
 * 3. Register callback functions using 'stte_main()'. (The registered callback functions will be called when the STTE service users request the STTE services.) <br>
 * 4. Use 'service_app_main()' for working STTE. <br>
 *
 * <b>B. Optional STTE services</b> <br>
 * Unlike the required STTE services, these services are optional to operate STTE. The followings are optional STTE services. <br>
 * - receive/provide the private data <br>
 *
 * If the STTE developers want to provide the above services, use the following APIs and implement the corresponding callback functions: <br>
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>DESCRIPTION</th>
 * <th>CORRESPONDING CALLBACK</th>
 * </tr>
 * <tr>
 * <td>stte_set_private_data_set_cb()</td>
 * <td>Sets a callback function for receiving the private data from the engine service user.</td>
 * <td>stte_private_data_set_cb()</td>
 * </tr>
 * <tr>
 * <td>stte_set_private_data_requested_cb()</td>
 * <td>Sets a callback function for providing the private data to the engine service user.</td>
 * <td>stte_private_data_requested_cb()</td>
 * </tr>
 * </table>
 * <br>
 *
 * Using the above APIs, the STTE developers can register the optional callback functions respectively.
 * (For normal operation, put those APIs before 'service_app_main()' starts.)
 *
 * Unlike callback functions, the following APIs are functions for getting and sending data. The STTE developers can use these APIs when they implement STTE services: <br>
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>stte_send_result()</td>
 * <td>Sends the recognition result to the engine service user.</td>
 * </tr>
 * <tr>
 * <td>stte_send_error()</td>
 * <td>Sends the error to the engine service user.</td>
 * </tr>
 * <tr>
 * <td>stte_send_speech_status()</td>
 * <td>Sends the speech status to the engine service user when STT engine notifies the change of the speech status.</td>
 * </tr>
 * </table>
 *
 *
 * @section CAPI_UIX_STTE_MODULE_FEATURE Related Features
 * This API is related with the following features:<br>
 *  - http://tizen.org/feature/speech.recognition<br>
 *  - http://tizen.org/feature/microphone<br>
 *
 * It is recommended to design feature related codes in your application for reliability.<br>
 * You can check if a device supports the related features for this API by using @ref CAPI_SYSTEM_SYSTEM_INFO_MODULE, thereby controlling the procedure of your application.<br>
 * To ensure your application is only running on the device with specific features, please define the features in your manifest file using the manifest editor in the SDK.<br>
 * More details on featuring your application can be found from <a href="https://developer.tizen.org/development/tools/native-tools/manifest-text-editor#feature"><b>Feature Element</b>.</a>
 *
 */

#endif /* __TIZEN_UIX_STT_ENGINE_DOC_H__ */

