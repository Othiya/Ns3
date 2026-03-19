# Install script for directory: /Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/build/lib/libns3.45-core-default.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-core-default.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-core-default.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-core-default.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-core-default.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/build/include/ns3/core-config.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/int64x64-128.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/helper/csv-reader.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/helper/event-garbage-collector.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/helper/random-variable-stream-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/abort.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/ascii-file.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/ascii-test.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/assert.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/attribute-accessor-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/attribute-construction-list.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/attribute-container.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/attribute-helper.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/attribute.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/boolean.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/breakpoint.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/build-profile.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/calendar-scheduler.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/callback.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/command-line.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/config.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/default-deleter.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/default-simulator-impl.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/demangle.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/deprecated.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/des-metrics.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/double.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/enum.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/event-id.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/event-impl.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/fatal-error.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/fatal-impl.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/fd-reader.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/environment-variable.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/global-value.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/hash-fnv.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/hash-function.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/hash-murmur3.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/hash.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/heap-scheduler.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/int64x64-double.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/int64x64.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/integer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/length.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/list-scheduler.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/log-macros-disabled.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/log-macros-enabled.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/log.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/make-event.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/map-scheduler.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/math.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/names.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/node-printer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/nstime.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/object-base.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/object-factory.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/object-map.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/object-ptr-container.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/object-vector.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/object.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/pair.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/pointer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/priority-queue-scheduler.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/ptr.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/random-variable-stream.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/rng-seed-manager.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/rng-stream.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/scheduler.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/show-progress.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/shuffle.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/simple-ref-count.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/simulation-singleton.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/simulator-impl.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/simulator.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/singleton.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/string.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/synchronizer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/system-path.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/system-wall-clock-ms.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/system-wall-clock-timestamp.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/test.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/time-printer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/timer-impl.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/timer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/trace-source-accessor.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/traced-callback.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/traced-value.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/trickle-timer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/tuple.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/type-id.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/type-name.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/type-traits.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/uinteger.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/uniform-random-bit-generator.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/valgrind.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/vector.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/warnings.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/watchdog.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/realtime-simulator-impl.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/wall-clock-synchronizer.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/val-array.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/src/core/model/matrix-array.h"
    "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/build/include/ns3/core-module.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/farihanisraq/ns-allinone-3.45/ns-3.45/cmake-cache/src/core/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
