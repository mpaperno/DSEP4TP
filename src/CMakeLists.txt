cmake_minimum_required(VERSION 3.24)

set (VERSION_STR 1.1.0.1)
set (VERSION_NUM 1010001)
set (VERSION_SFX -beta1)
set (PROJECT_FULL_NAME  "Dynamic Script Engine Plugin for Touch Portal")
set (PROJECT_SHORT_NAME "Dynamic Script Engine")
set (PROJECT_ORG_NAME   "DynamicScriptEngine")
set (PROJECT_DOMAIN "dse.tpp.max.paperno.us")
set (TP_PLUGIN_ID   "us.paperno.max.tpp.dse")

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
  version.h
  main.cpp
  common.h
  Plugin.h
  Plugin.cpp
  ConnectorData.h
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
endif()

if (UNIX OR MINGW AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs )
endif()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
  Core
  Network
  Qml
  Sql
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Qml
  Qt${QT_VERSION_MAJOR}::Sql
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Qt${QT_VERSION_MAJOR}Qml_PRIVATE_INCLUDE_DIRS})

##  Install

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
  set(deploy_command "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-translations "${executable_path}")
  install(CODE "execute_process(COMMAND ${deploy_command})" COMPONENT Runtime)
  cmake_path(SET ssl_path "$ENV{QTHOME}/Tools/OpenSSL/Win_x64/bin")
  install(
    FILES "${ssl_path}/libcrypto-1_1-x64.dll" "${ssl_path}/libssl-1_1-x64.dll"
    TYPE BIN
  )

elseif(isLinux)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    CMAKE_BUILD_RPATH_USE_ORIGIN TRUE
    INSTALL_RPATH "$ORIGIN"
  )
  #install(TARGETS ${PROJECT_NAME} RUNTIME_DEPENDENCIES DESTINATION bin)  # all C++ libs
  set(qt_ver ${QT_VERSION_MAJOR})
  set(qt_libs "")
  foreach(qt_lib
    Qt${qt_ver}Core.so.${qt_ver} Qt${qt_ver}Network.so.${qt_ver} Qt${qt_ver}Qml.so.${qt_ver} Qt${qt_ver}Sql.so.${qt_ver}
    icui18n.so.56 icuuc.so.56 icudata.so.56
  )
    list(APPEND qt_libs "${CMAKE_PREFIX_PATH}/lib/lib${qt_lib}")
  endforeach()
  install(CODE
    "file(INSTALL ${qt_libs} DESTINATION \"${CMAKE_INSTALL_PREFIX}/bin\" FOLLOW_SYMLINK_CHAIN)"
    COMPONENT Runtime)
  install(DIRECTORY "${CMAKE_PREFIX_PATH}/plugins/networkinformation" TYPE BIN)
  install(DIRECTORY "${CMAKE_PREFIX_PATH}/plugins/tls" TYPE BIN)
  install(DIRECTORY "${CMAKE_PREFIX_PATH}/plugins/sqldrivers" TYPE BIN)

else()  # Mac
  set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    INSTALL_RPATH "@executable_path/../Frameworks/"
  )
  # macdeployqt is only needed when completely rebuilding the bundle; It also adds a bunch of GUI QApplication junk we don't need.
  set(bundle_path "${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.app")
  add_custom_target(deployqt
    COMMAND ${MACDEPLOYQT_EXECUTABLE} "${bundle_path}"
    COMMAND rm -f -r "${bundle_path}/Contents/Frameworks/QtDBus.framework"
    COMMAND rm -f -r "${bundle_path}/Contents/Frameworks/QtGui.framework"
    COMMAND rm -f -r "${bundle_path}/Contents/Frameworks/QtSvg.framework"
    COMMAND rm -f -r "${bundle_path}/Contents/Frameworks/QtWidgets.framework"
    COMMAND rm -f -r "${bundle_path}/Contents/Plugins/iconengines"
    COMMAND rm -f -r "${bundle_path}/Contents/Plugins/imageformats"
    COMMAND rm -f -r "${bundle_path}/Contents/Plugins/styles"
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
  )
endif()
