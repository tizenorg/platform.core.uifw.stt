SET(SRCS
	stt.c
	stt_client.c
	stt_dbus.c
	../common/stt_config_mgr.c
	../common/stt_config_parser.c
)

SET(FILE_SRCS
	stt_file.c
	stt_file_client.c
	../common/stt_config_mgr.c
	../common/stt_config_parser.c
	../common/stt_engine.c
	../common/stt_network.c
)

set(SETTING_SRCS
	stt_setting.c
	../common/stt_config_mgr.c
	../common/stt_config_parser.c
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/server)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

## Add definitions ##
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")

## stt library ##
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

## stt file library ##
ADD_LIBRARY("${PROJECT_NAME}_file" SHARED ${FILE_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_file" ${pkgs_LDFLAGS})

## stt-setting library ##
ADD_LIBRARY("${PROJECT_NAME}_setting" SHARED ${SETTING_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_setting" ${pkgs_LDFLAGS})

## Install library files ##
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS "${PROJECT_NAME}_file" DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS "${PROJECT_NAME}_setting" DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
