CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libstt)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED 
	glib-2.0 dbus-1 
	vconf dlog mm-player mm-common mm-camcorder openssl ecore
)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)
