cmake_minimum_required(VERSION 3.24)

set (VERSION_STR 1.0.0.1)
set (VERSION_NUM 1000001)
set (VERSION_SFX -beta1)
set (PROJECT_FULL_NAME "Dynamic Script Engine Plugin for Touch Portal")
set (PROJECT_SHORT_NAME "Dynamic Script Engine")
set (PROJECT_DOMAIN "tpp.max.paperno.us")
set (TP_PLUGIN_ID "us.paperno.max.tpp.dsep")

project(
  DSEP4TP
  VERSION ${VERSION_STR}
  DESCRIPTION ${PROJECT_FULL_NAME}
  LANGUAGES CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_path(SET SRCPATH "${PROJECT_SOURCE_DIR}")
cmake_path(SET DOXPATH "${CMAKE_SOURCE_DIR}/../doc/doxygen")
cmake_path(SET BLDPATH "${CMAKE_SOURCE_DIR}/../build")
cmake_path(SET DSTPATH "${CMAKE_SOURCE_DIR}/../dist/${CMAKE_BUILD_TYPE}")
cmake_path(SET SSLPATH "$ENV{QTHOME}/Tools/OpenSSL/Win_x64/bin")

set(CMAKE_INSTALL_PREFIX ${DSTPATH})

find_package(QT NAMES Qt5 Qt6 REQUIRED)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
  Core
  Network
  Concurrent
  Qml
)

#add_subdirectory(monitor)

add_executable(${PROJECT_NAME})

#set_target_properties(${PROJECT_NAME} PROPERTIES
#  WIN32_EXECUTABLE TRUE
#  MACOSX_BUNDLE TRUE
#)

target_sources(${PROJECT_NAME} PRIVATE
  version.h
  main.cpp
  common.h
  Plugin.h
  Plugin.cpp
  DynamicScript.h
  ScriptEngine.h
  ScriptEngine.cpp
  Logger.h
  Logger.cpp
  TPClientQt.h
  TPClientQt.cpp
  RunGuard.h
  utils.h

  ScriptingLibrary/AbortController.h
  ScriptingLibrary/File.h
  ScriptingLibrary/FS.h
  ScriptingLibrary/Dir.h
  ScriptingLibrary/Process.h
  ScriptingLibrary/Util.h

  resources/resources.qrc
)

if (QT_VERSION_MAJOR STREQUAL "6")
  target_sources(${PROJECT_NAME} PRIVATE
    ScriptingLibrary/DOMException.h
    ScriptingLibrary/DOMException.cpp
    ScriptingLibrary/XmlHttpRequest.h
    ScriptingLibrary/XmlHttpRequest.cpp
    #  ScriptingLibrary/qv4httpheaders.h
    #  ScriptingLibrary/qv4httpheaders.cpp
  )
endif()

if (WIN32)
  target_sources(${PROJECT_NAME} PRIVATE resources/win.rc )
endif()

configure_file(version.h.in version.h @ONLY)
configure_file(${DOXPATH}/Doxyfile.version.in ${DOXPATH}/Doxyfile.version @ONLY)
configure_file(${BLDPATH}/version.json.in ${BLDPATH}/version.json @ONLY)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ScriptingLibrary)
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt${QT_VERSION_MAJOR}Qml_PRIVATE_INCLUDE_DIRS})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  QT_USE_QSTRINGBUILDER
  QT_MESSAGELOGCONTEXT
  #QT_NO_KEYWORDS
  #QT_QML_DEBUG
)

if (MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS )
endif()

if (WIN32 AND NOT MSVC)
  target_link_libraries(${PROJECT_NAME} stdc++fs )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Concurrent
  Qt${QT_VERSION_MAJOR}::Qml
)

##  Install

install(TARGETS ${PROJECT_NAME}
  BUNDLE  DESTINATION .
  RUNTIME DESTINATION bin
)

if (WIN32)
  install(
    FILES "${SSLPATH}/libcrypto-1_1-x64.dll" "${SSLPATH}/libssl-1_1-x64.dll"
    TYPE BIN
  )
endif()

#if(APPLE)
#  set(executable_path "$<TARGET_FILE_NAME:${PROJECT_NAME}>.app")
#else()
  set(executable_path "\${QT_DEPLOY_BIN_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>")
#endif()

set(deploy_script "${CMAKE_CURRENT_BINARY_DIR}/deploy_${PROJECT_NAME}.cmake")

file(GENERATE OUTPUT ${deploy_script} CONTENT "
include(\"${QT_DEPLOY_SUPPORT}\")
set(__qt_deploy_tool_extra_options --no-compiler-runtime --no-translations)
qt_deploy_runtime_dependencies(
    EXECUTABLE \"${executable_path}\"
    PLUGINS_DIR bin
#    NO_TRANSLATIONS
)
")

#qt_generate_deploy_app_script(
#    TARGET ${PROJECT_NAME}
#    FILENAME_VARIABLE deploy_script
#    NO_UNSUPPORTED_PLATFORM_ERROR
#)

install(SCRIPT ${deploy_script})

#qt_add_qml_module(${PROJECT_NAME}
#    URI qml
#    VERSION 1.0
#    QML_FILES
#      qml/root.qml
#)
