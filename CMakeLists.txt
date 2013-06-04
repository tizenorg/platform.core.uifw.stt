CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libstt)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED 
	glib-2.0 dbus-1 vconf dlog ecore ecore-file capi-media-audio-io capi-media-sound-manager
)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)

INSTALL(FILES ${CMAKE_SOURCE_DIR}/stt-server.rule DESTINATION  /opt/etc/smack/accesses.d)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/sysinfo-stt.xml DESTINATION /etc/config)
