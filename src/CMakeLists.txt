
# Most of the sources are not in the core
SET(COMMON_SOURCES
	coreutils/closeout.c
	coreutils/close-stream.c
	coreutils/error.c
	coreutils/fpending.c
	coreutils/version-etc.c
	coreutils/xstrtoumax.c
	portability/safe_write.c
	portability/strmov.c
	buffer.c
	stdout.c
)
SET(SERVER_SOURCES
	${COMMON_SOURCES}
	coreutils/fadvise.c
	coreutils/freopen-safer.c
	coreutils/xfreopen.c
	pipes.c
	sockets.c
	tailserver.c
)
SET(CLIENT_SOURCES
	${COMMON_SOURCES}
	coreutils/xstrtod.c
	clientconn.c
	clientloop.c
	clientpipes.c
	evsleep.c
	tailclient.c
)
# These aren't used because libev is prefered so that libev can handle signals while sleeping.
#	coreutils/dtotimespec.c
#	coreutils/xnanosleep.c

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/coreutils
	${CMAKE_CURRENT_SOURCE_DIR}/portability
)

ADD_EXECUTABLE(tailserver ${SERVER_SOURCES})
TARGET_LINK_LIBRARIES(tailserver ev)
INSTALL_TARGETS(tailserver DESTINATION ${INSTALL_BINDIR})

ADD_EXECUTABLE(tailclient ${CLIENT_SOURCES})
TARGET_LINK_LIBRARIES(tailclient ev)
INSTALL_TARGETS(tailclient DESTINATION ${INSTALL_BINDIR})

FOREACH(tool glibtoolize libtoolize aclocal autoconf autoheader automake gtar 
 tar git)
 STRING(TOUPPER ${tool}  TOOL)
 FIND_PROGRAM(${TOOL}_EXECUTABLE ${tool} DOC "path to the executable")
 MARK_AS_ADVANCED(${TOOL}_EXECUTABLE)
ENDFOREACH()

CONFIGURE_FILE(
  ${CMAKE_SOURCE_DIR}/cmake/make_dist.cmake.in ${CMAKE_BINARY_DIR}/make_dist.cmake @ONLY)

ADD_CUSTOM_TARGET(dist 
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/make_dist.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

ADD_CUSTOM_TARGET(distclean
  COMMAND ${CMAKE_COMMAND} -E echo  WARNING: distclean target is not functional
  COMMAND ${CMAKE_COMMAND} -E echo  "Try 'rm -rf build && sh configure' instead"
  VERBATIM
  )

IF(INSTALL_LAYOUT STREQUAL "STANDALONE")
  ADD_CUSTOM_TARGET(show-dist-name
    COMMAND ${CMAKE_COMMAND} -E echo "${CPACK_PACKAGE_FILE_NAME}"
  )
ENDIF()
