SET(SRCS
	sttd_client_data.c
	sttd_config.c
	sttd_dbus_server.c
	sttd_dbus.c
	sttd_engine_agent.c
	stte.c
	sttd_recorder.c
	sttd_server.c
	../common/stt_config_mgr.c
	../common/stt_config_parser.c
	../common/stt_engine.c
	../common/stt_network.c
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/common)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/server)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE")
#SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -fPIE")
#SET(CMAKE_C_FLAGS_RELEASE "-O2 -fPIE")
#SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

## Executable ##
ADD_LIBRARY("${PROJECT_NAME}_engine" SHARED ${SRCS})
#TARGET_LINK_LIBRARIES("${PROJECT_NAME}_engine" -ldl -lm ${pkgs_LDFLAGS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_engine" ${pkgs_LDFLAGS})

## Install
INSTALL(TARGETS "${PROJECT_NAME}_engine" DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)

