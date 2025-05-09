cmake_minimum_required(VERSION 3.14)

project(TPClientQt
  DESCRIPTION "Network client for Touch Portal plugin API."
  LANGUAGES CXX
)

option(TPClient_BUILD_SHARED_LIBS "Build project as a shared (DLL) library when ON, or as static lib when OFF." TRUE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  TPClientQt.h
  TPClientQt.cpp
)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE TPCLIENT_BUILD_DLL)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
  QT_USE_QSTRINGBUILDER
  QT_MESSAGELOGCONTEXT
  QT_NO_KEYWORDS
)

find_package(QT NAMES Qt5 Qt6 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
  Core
  Network
)
target_link_libraries(${PROJECT_NAME}
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
)
