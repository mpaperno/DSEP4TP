cmake_minimum_required(VERSION 3.24)

set (VERSION_STR 1.2.1.0)
set (VERSION_SFX "")
set (PROJECT_FULL_NAME  "Dynamic Script Engine Plugin for Touch Portal")
set (PROJECT_SHORT_NAME "Dynamic Script Engine")
set (PROJECT_ORG_NAME   "DynamicScriptEngine")
set (PROJECT_DOMAIN "dse.tpp.max.paperno.us")
set (TP_PLUGIN_ID   "us.paperno.max.tpp.dse")

project(
  DSEP4TP
  VERSION ${VERSION_STR}
  HOMEPAGE_URL "https://dse.tpp.max.paperno.us"
  DESCRIPTION "${PROJECT_SHORT_NAME} - A complete JavaScript scripting environment for Touch Portal."
  LANGUAGES CXX
)

math(EXPR VERSION_NUM
  "((${PROJECT_VERSION_MAJOR} & 0xFF) << 24) | ((${PROJECT_VERSION_MINOR} & 0xFF) << 16) | ((${PROJECT_VERSION_PATCH} & 0xFF) << 8) | (${PROJECT_VERSION_TWEAK} & 0xFF)"
  OUTPUT_FORMAT HEXADECIMAL
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

cmake_path(SET SRCPATH "${PROJECT_SOURCE_DIR}")
cmake_path(SET DOXPATH "${CMAKE_SOURCE_DIR}/../doc/doxygen")
cmake_path(SET BLDPATH "${CMAKE_SOURCE_DIR}/../build")

set(isWin FALSE)
set(isMac FALSE)
set(isLinux FALSE)

if(WIN32)
  set(PLATFORM_OS Windows)
  set(isWin TRUE)
elseif(APPLE)
  set(PLATFORM_OS MacOS)
  set(isMac TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(PLATFORM_OS Linux)
  set(isLinux TRUE)
else()
  message(FATAL_ERROR "Platform not supported.")
endif()

find_package(QT NAMES Qt5 Qt6 REQUIRED)

#add_subdirectory(monitor)

add_executable(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
  MACOSX_BUNDLE TRUE
)

target_sources(${PROJECT_NAME} PRIVATE
  common.h
  dse_strings.h
  event_utils.h
  main.cpp
  utils.h
  version.h
  version.h.in

  ConnectorData.h
  DSE_NS.h
  DSE.h
  DSE.cpp
  DynamicScript.h
  DynamicScript.cpp
  JSError.h
  Logger.h
  Logger.cpp
  Plugin.h
  Plugin.cpp
  RunGuard.h
  ScriptEngine.h
  ScriptEngine.cpp

  ScriptingLibrary/AbortController.h
  ScriptingLibrary/Clipboard.h
  ScriptingLibrary/File.h
  ScriptingLibrary/FileInfo.h
  ScriptingLibrary/FS.h
  ScriptingLibrary/Dir.h
  ScriptingLibrary/RunProcess.h
  ScriptingLibrary/TPAPI.h
  ScriptingLibrary/Util.h

  resources/resources.qrc
)

if (QT_VERSION_MAJOR STREQUAL "6")
  target_sources(${PROJECT_NAME} PRIVATE
    ScriptingLibrary/DOMException.h
    ScriptingLibrary/DOMException.cpp
    ScriptingLibrary/XmlHttpRequest.h
    ScriptingLibrary/XmlHttpRequest.cpp
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

target_compile_definitions(${PROJECT_NAME} PRIVATE
  QT_USE_QSTRINGBUILDER
  QT_MESSAGELOGCONTEXT
  #QT_NO_KEYWORDS
  #QT_QML_DEBUG
)

if (MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS )
elseif (UNIX OR MINGW AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs )
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
  Core
  Network
  Qml
  Sql
  Gui
  Svg
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Qml
  Qt${QT_VERSION_MAJOR}::Sql
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Svg
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Qt${QT_VERSION_MAJOR}Qml_PRIVATE_INCLUDE_DIRS})

## TPClientQt
add_subdirectory("${SRCPATH}/TPClientQt" EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PRIVATE TPClientQt)

##  Install

cmake_policy(SET CMP0177 NEW)  ## v3.31 - install() DESTINATION paths are normalized.

cmake_path(SET INSTALL_DEST_REL "${CMAKE_SOURCE_DIR}/../dist/${PLATFORM_OS}/${PROJECT_NAME}")
cmake_path(SET INSTALL_DEST_DBG "${CMAKE_SOURCE_DIR}/../dist/${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(INSTALL_DEST ${INSTALL_DEST_REL})
else()
  set(INSTALL_DEST ${INSTALL_DEST_DBG})
endif()
set(CMAKE_INSTALL_PREFIX "${INSTALL_DEST}")
#set(CMAKE_INSTALL_PREFIX "$<IF:$<CONFIG:Release>,\${INSTALL_DEST_REL},\${INSTALL_DEST_DBG}>")

install(TARGETS ${PROJECT_NAME}
  BUNDLE  DESTINATION .
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ
  RUNTIME DESTINATION bin
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ
)

if (isWin)
  set(executable_path "${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>")
  set(deploy_command "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-translations --no-quick "${executable_path}")  # --no-qmltooling
  install(CODE "execute_process(COMMAND ${deploy_command})" COMPONENT Runtime)
  cmake_path(SET ssl_path "$ENV{QTHOME}/Tools/OpenSSLv3/Win_x64/bin")
  cmake_path(SET rt_path "c:/Windows/system32")
  install(
    FILES
      # "${CMAKE_PREFIX_PATH}/bin/Qt${QT_VERSION_MAJOR}Qml.dll"
      "${ssl_path}/libcrypto-3-x64.dll" "${ssl_path}/libssl-3-x64.dll"
      "${rt_path}/msvcp140.dll" "${rt_path}/msvcp140_1.dll"
      "${rt_path}/vcruntime140.dll" "${rt_path}/vcruntime140_1.dll"
    TYPE BIN
  )

elseif(isLinux)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    CMAKE_BUILD_RPATH_USE_ORIGIN TRUE
    INSTALL_RPATH "$ORIGIN/lib"
  )
  set(install_dest "${CMAKE_INSTALL_PREFIX}/bin")
  #install(TARGETS ${PROJECT_NAME} RUNTIME_DEPENDENCIES DESTINATION bin)  # all C++ libs
  set(qt_ver ${QT_VERSION_MAJOR})
  set(qt_libs "")
  foreach(qt_lib
    Qt${qt_ver}Core.so.${qt_ver} Qt${qt_ver}Network.so.${qt_ver} Qt${qt_ver}Qml.so.${qt_ver} Qt${qt_ver}Sql.so.${qt_ver}
    Qt${qt_ver}Gui.so.${qt_ver} Qt${qt_ver}Svg.so.${qt_ver} Qt${qt_ver}DBus.so.${qt_ver} Qt${qt_ver}OpenGL.so.${qt_ver}
    Qt${qt_ver}XcbQpa.so.${qt_ver} Qt${qt_ver}EglFSDeviceIntegration.so.${qt_ver} Qt${qt_ver}EglFsKmsSupport.so.${qt_ver}
    Qt${qt_ver}WaylandClient.so.${qt_ver} Qt${qt_ver}WaylandEglClientHwIntegration.so.${qt_ver} Qt${qt_ver}WlShellIntegration.so.${qt_ver}
    icui18n.so.56 icuuc.so.56 icudata.so.56
  )
    list(APPEND qt_libs "${CMAKE_PREFIX_PATH}/lib/lib${qt_lib}")
  endforeach()
  install(CODE "
    file(INSTALL ${qt_libs} DESTINATION \"${install_dest}/lib\" FOLLOW_SYMLINK_CHAIN)
    file(WRITE \"${install_dest}/qt.conf\" \"[Paths]\nPlugins=plugins\nLibraries=lib\n\")
    " COMPONENT Runtime)
  foreach(qt_plugin
    networkinformation
    iconengines
    imageformats
    platforms
    platforminputcontexts
    tls
    sqldrivers
    xcbglintegrations
  )
    install(DIRECTORY "${CMAKE_PREFIX_PATH}/plugins/${qt_plugin}" DESTINATION "${install_dest}/plugins")
  endforeach()

else()  # Mac
  set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    INSTALL_RPATH "@executable_path/../Frameworks/"
  )
  # macdeployqt is only needed when completely rebuilding the bundle; It also adds a bunch of GUI QApplication junk we don't need.
  set(bundle_path "${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.app")
  add_custom_target(deployqt
    COMMAND ${MACDEPLOYQT_EXECUTABLE} "${bundle_path}"
    COMMAND rm -f -r "${bundle_path}/Contents/Frameworks/QtWidgets.framework"
    COMMAND rm -f -r "${bundle_path}/Contents/Plugins/styles"
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
  )
endif()
