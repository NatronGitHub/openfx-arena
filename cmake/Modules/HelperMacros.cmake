macro(INKSCAPE_PKG_CONFIG_FIND PREFIX MODNAME VERSION PATH_NAME PATH_SUFFIXE LIB_NAME)
    if(VERSION)
	pkg_check_modules(_${PREFIX} ${MODNAME}${VERSION})
    else(VERSION)
	pkg_check_modules(_${PREFIX} ${MODNAME})
    endif(VERSION)

    find_path(${PREFIX}_INCLUDE_DIR
	NAMES
	    ${PATH_NAME}
	PATHS
	    ${_${PREFIX}_INCLUDEDIR}
	    /usr/include
	    /usr/local/include
	    /opt/local/include
	    /sw/include
	    $ENV{DEVLIBS_PATH}//include//
	PATH_SUFFIXES
	    ${PATH_SUFFIXE}
	)

    find_library(${PREFIX}_LIBRARY
	NAMES
	    ${LIB_NAME}
	PATHS
	    ${_${PREFIX}_LIBDIR}
	    /usr/lib
	    /usr/local/lib
	    /opt/local/lib
	    /sw/lib
	)

    if (${PREFIX}_LIBRARY)
	set(${PREFIX}_FOUND TRUE)
	set(${PREFIX}_VERSION ${_${PREFIX}_VERSION})
    endif (${PREFIX}_LIBRARY)

    set(${PREFIX}_INCLUDE_DIRS
	${${PREFIX}_INCLUDE_DIR}
	)

    if (${PREFIX}_FOUND)
	set(${PREFIX}_LIBRARIES
	    ${${PREFIX}_LIBRARIES}
	    ${${PREFIX}_LIBRARY}
	    )
    endif (${PREFIX}_FOUND)

    if (${PREFIX}_INCLUDE_DIRS AND ${PREFIX}_LIBRARIES)
	set(${PREFIX}_FOUND TRUE)
    endif (${PREFIX}_INCLUDE_DIRS AND ${PREFIX}_LIBRARIES)

    if (${PREFIX}_FOUND)
	if (NOT ${PREFIX}_FIND_QUIETLY)
	    message(STATUS "Found ${MODNAME}: ${${PREFIX}_LIBRARIES}")
	endif (NOT ${PREFIX}_FIND_QUIETLY)
    else (${PREFIX}_FOUND)
	if (${PREFIX}_FIND_REQUIRED)
	    message(FATAL_ERROR "Could not find ${MODNAME}")
	endif (${PREFIX}_FIND_REQUIRED)
    endif (${PREFIX}_FOUND)

    # show the <PREFIX>_INCLUDE_DIRS and <PREFIX>_LIBRARIES variables only in the advanced view
    mark_as_advanced(${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES)
endmacro()
