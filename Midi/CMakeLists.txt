# Midi.ofx

cmake_minimum_required(VERSION 3.1)
project(Midi CXX)

include(FindOpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(OPENFX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../OpenFX)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${OPENFX_PATH}/include)
include_directories(${OPENFX_PATH}/Support/include)
include_directories(${OPENFX_PATH}/Support/Plugins/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/RtMidi)

# Define "DEBUG" on debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# Add OpenFX sources
set(OFX_SUPPORT_HEADER_DIR "${OPENFX_PATH}/Support/include")
set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_PATH}/Support/Library")
file(GLOB OPENFX_SOURCES
    "${OFX_SUPPORT_HEADER_DIR}/*.h"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsCore.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsImageEffect.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsInteract.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsLog.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsMultiThread.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsParams.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsProperty.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsPropertyValidation.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/*.h"
)

# Add sources
file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/MidiInOFX.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RtMidi/RtMidi.cpp"
)

# Add resources
file(GLOB RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/net.fxarena.openfx.MidiIn.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/net.fxarena.openfx.MidiIn.svg"
)

# jack
find_library(JACK_LIB jack)
find_package(PkgConfig)
pkg_check_modules(jack jack)
if(jack_FOUND)
    add_definitions(-D__UNIX_JACK__)
    include_directories(${jack_INCLUDEDIR})
endif()

# alsa
if(UNIX AND NOT APPLE)
    find_package(ALSA)
    if(ALSA_FOUND)
        add_definitions(-D__LINUX_ALSA__)
        include_directories(${ALSA_INCLUDE_DIR})
    endif()
endif()

# pthreads
if(jack_FOUND OR ALSA_FOUND)
    find_package(Threads REQUIRED
                 CMAKE_THREAD_PREFER_PTHREAD
                 THREADS_PREFER_PTHREAD_FLAG)
endif()

# win32
if(WIN32)
    add_definitions(-D__WINDOWS_MM__)
endif()

# apple
if(APPLE)
    add_definitions(-D__MACOSX_CORE__)
endif()

# Add binary
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${OPENFX_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".ofx")

# Add extra flags to the link step of the plugin
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(MINGW)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-shared -fvisibility=hidden -Xlinker --version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
# Linux & FreeBSD
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
endif()

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    OFX_EXTENSIONS_VEGAS
    OFX_EXTENSIONS_NUKE
    OFX_EXTENSIONS_NATRON
    OFX_EXTENSIONS_TUTTLE
    OFX_SUPPORTS_OPENGLRENDER
    NOMINMAX
)

target_link_libraries(
    ${PROJECT_NAME}
    ${OPENGL_gl_LIBRARY}
)

# jack
if(jack_FOUND)
    target_link_libraries(${PROJECT_NAME} ${JACK_LIB} Threads::Threads)
endif()

# alsa
if(UNIX AND NOT APPLE)
    if(ALSA_FOUND)
        target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARY} Threads::Threads)
    endif()
endif()

# win32
if(WIN32)
    target_link_libraries(${PROJECT_NAME} winmm)
endif()

# apple
if(APPLE)
    target_link_libraries(-framework CoreServices -framework CoreAudio -framework CoreMIDI -framework CoreFoundation)
endif()

# Find and set the arch name.
# http://openeffects.org/documentation/reference/ch02s02.html
set(OFX_ARCH UNKNOWN)
# Win
if(MINGW)
    set(OFX_ARCH "Win32")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Win64")
    endif()
# Visual Studio
elseif(MSVC)
    set(OFX_ARCH "Win32")
    if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL "IA64") OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64"))
        set(OFX_ARCH "Win64")
    endif()
# FreeBSD
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(OFX_ARCH "FreeBSD-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "FreeBSD-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH FreeBSD-arm-64)
        else()
            set(OFX_ARCH FreeBSD-arm)
        endif()
    endif()
# MacOS
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OFX_ARCH "MacOS")
# Linux
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OFX_ARCH "Linux-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Linux-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH Linux-arm-64)
        else()
            set(OFX_ARCH Linux-arm)
        endif()
    endif()
endif()

# Install
set(OFX_ARCH_NAME ${OFX_ARCH} CACHE STRING "OpenFX target OS and architecture")
if("${OFX_ARCH_NAME}" EQUAL "UNKNOWN")
    message(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
else()
    install(
        TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
        LIBRARY DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
    )
    install(
        FILES ${RESOURCES}
        DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/Resources
    )
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents
    )
endif()
