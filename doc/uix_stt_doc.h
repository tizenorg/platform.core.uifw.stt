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


#ifndef __TIZEN_UIX_STT_DOC_H__
#define __TIZEN_UIX_STT_DOC_H__

/**
 * @defgroup CAPI_UIX_STT_MODULE STT
 * @ingroup CAPI_UIX_FRAMEWORK
 * @brief The @ref CAPI_UIX_STT_MODULE API provides functions to recognize the speech.
 * 
 * @section CAPI_UIX_STT_MODULE_HEADER Required Header
 *   \#include <stt.h>
 * 
 * @section CAPI_UIX_STT_MODULE_OVERVIEW Overview
 * A main function of Speech-To-Text (below STT) API recognizes sound data recorded by users.
 * After choosing a language, applications will start recording and recognizing.
 * After recording, the applications will receive the recognized result.
 *
 * To use of STT, use the following steps:<br>
 * 1. Create a handle <br>
 * 2. Register callback functions for notifications <br> 
 * 3. Prepare stt-daemon asynchronously <br>
 * 4. Start recording for recognition <br>
 * 5. Stop recording <br>
 * 6. Get result after processing <br>
 * 7. Destroy a handle <br>
 *
 * The STT has a client-server for the service of multi-applications.
 * The STT daemon as a server always works in the background for the STT service.
 * If the daemon is not working, client library will invoke it and client will communicate with it.
 * The daemon has engines and the recorder so client does not have the recorder itself.
 * Only the client request commands to the daemon for the service.
 *
 * @section CAPI_STT_MODULE_STATE_DIAGRAM State Diagram
 * The following diagram shows the life cycle and the states of the STT.
 *
 * @image html capi_uix_stt_state_diagram.png "State diagram"
 *
 * @section CAPI_STT_MODULE_STATE_TRANSITIONS State Transitions
 * 
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>PRE-STATE</th>
 * <th>POST-STATE</th>
 * <th>SYNC TYPE</th>
 * </tr>
 * <tr>
 * <td>stt_prepare()</td>
 * <td>Created</td>
 * <td>Ready</td>
 * <td>ASYNC</td>
 * </tr>
 * <tr>
 * <td>stt_start()</td>
 * <td>Ready</td>
 * <td>Recording</td>
 * <td>ASYNC</td>
 * </tr>
 * <tr>
 * <td>stt_stop()</td>
 * <td>Recording</td>
 * <td>Processing</td>
 * <td>ASYNC</td>
 * </tr>
 * </tr>
 * <tr>
 * <td>stt_cancel()</td>
 * <td>Recording or Processing</td>
 * <td>Ready</td>
 * <td>SYNC</td>
 * </tr>
 * </table>
 * 
 * @section CAPI_STT_MODULE_STATE_DEPENDENT_FUNCTION_FUNCTION_CALLS State Dependent Function Calls
 * The following table shows state-dependent function calls. It is forbidden to call functions listed below in wrong states.
 * Violation of this rule may result in an unpredictable behavior.
 * 
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>VALID STATES</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>stt_create()</td>
 * <td>None</td>
 * <td>All functions must be called after stt_create().</td>
 * </tr>
 * <tr>
 * <td>stt_destroy()</td>
 * <td>Created, Ready, Recording, Processing</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_prepare()</td>
 * <td>Created</td>
 * <td>This function works asynchronously. If daemon fork is failed, application gets the error callback.</td>
 * </tr>
 * <tr>
 * <td>stt_unprepare()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_foreach_supported_engines()</td>
 * <td>Created</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_get_engine()</td>
 * <td>Created</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_set_engine()</td>
 * <td>Created</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_foreach_supported_languages()</td>
 * <td>Created, Ready, Recording, Processing</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_get_default_language()</td>
 * <td>Created, Ready, Recording, Processing</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_get_state()</td>
 * <td>Created, Ready, Recording, Processing</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_is_recognition_type_supported()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_set_silence_detection()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_set_start_sound()<br>stt_unset_start_sound()<br>stt_set_stop_sound()<br>stt_unset_stop_sound()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_start()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_stop()</td>
 * <td>Recording</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_cancel()</td>
 * <td>Recording, Processing</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_get_recording_volume()</td>
 * <td>Recording</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>stt_foreach_detailed_result()</td>
 * <td>Processing</td>
 * <td>This must be called in stt_recognition_result_cb()</td>
 * </tr>
 * <tr>
 * <td>stt_set_recognition_result_cb()<br>stt_unset_recognition_result_cb()<br>
 * stt_set_state_changed_cb()<br>stt_unset_state_changed_cb()<br>stt_set_error_cb()<br>stt_unset_error_cb()<br>
 * stt_set_default_language_changed_cb()<br>stt_unset_default_language_changed_cb()
 * </td>
 * <td>Created</td>
 * <td>All callback function should be registered / unregistered in Created state</td>
 * </tr>
 * </table>
 * 
 * @section CAPI_UIX_STT_MODULE_FEATURE Related Features
 * This API is related with the following features:<br>
 *  - http://tizen.org/feature/speech.recognition<br>
 *  - http://tizen.org/feature/microphone<br>
 *
 * It is recommended to design feature related codes in your application for reliability.<br>
 * You can check if a device supports the related features for this API by using @ref CAPI_SYSTEM_SYSTEM_INFO_MODULE, thereby controlling the procedure of your application.<br>
 * To ensure your application is only running on the device with specific features, please define the features in your manifest file using the manifest editor in the SDK.<br>
 * More details on featuring your application can be found from <a href="../org.tizen.mobile.native.appprogramming/html/ide_sdk_tools/feature_element.htm"><b>Feature Element</b>.</a>
 * 
 */

#endif /* __TIZEN_UIX_STT_DOC_H__ */

