CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(stt-daemon)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}")
SET(LIBDIR "${PREFIX}/lib")
SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION 0.0.1)

SET(SRCS
	sttd_config.c
	sttd_client_data.c
	sttd_engine_agent.c
	sttd_server.c
	sttd_recorder.c
	sttd_network.c
	sttd_dbus_server.c
	sttd_dbus.c
	sttd_main.c
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${INCLUDEDIR})
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/../common")

## Dependent packages ##
#INCLUDE(FindPkgConfig)
#pkg_check_modules(pkgs REQUIRED 
#	glib-2.0 dbus-1 
#	vconf dlog mm-player mm-common mm-camcorder openssl
#)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

## Executable ##
ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -ldl -lm ${pkgs_LDFLAGS})

## Install
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sttp.h DESTINATION include)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sttd.conf DESTINATION lib/voice/stt/1.0)
