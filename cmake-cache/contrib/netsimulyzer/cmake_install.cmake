# Install script for directory: /Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "default")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/build/lib/libns3.45-netsimulyzer-default.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-netsimulyzer-default.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-netsimulyzer-default.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/build/lib"
      -add_rpath "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-netsimulyzer-default.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-netsimulyzer-default.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/area-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/building-configuration-container.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/building-configuration-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/logical-link-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/node-configuration-container.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/node-configuration-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/helper/throughput-sink-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/library/json.hpp"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/event-message.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/log-stream.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/logical-link.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/netsimulyzer-3D-models.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/netsimulyzer-ns3-compatibility.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/netsimulyzer-version.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/node-configuration.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/optional.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/optional-types.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/building-configuration.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/category-axis.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/category-value-series.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/color.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/color-palette.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/decoration.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/ecdf-sink.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/orchestrator.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/rectangular-area.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/series-collection.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/state-transition-sink.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/value-axis.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/xy-series.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/contrib/netsimulyzer/model/throughput-sink.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/build/include/ns3/netsimulyzer-module.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/cmake-cache/contrib/netsimulyzer/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
