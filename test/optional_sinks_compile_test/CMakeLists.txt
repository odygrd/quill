cmake_minimum_required(VERSION 3.12...4.0)
project(quill_optional_sinks_compile_test LANGUAGES CXX)

# Builds the sinks whose headers depend on optional system libraries, which the main build and
# test suite do not compile: SyslogSink, SystemdSink (libsystemd) and PrometheusSink
# (prometheus-cpp core and pull).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." quill)

find_path(SYSTEMD_INCLUDE_DIR systemd/sd-journal.h)
find_library(SYSTEMD_LIBRARY NAMES systemd)
if (NOT SYSTEMD_INCLUDE_DIR OR NOT SYSTEMD_LIBRARY)
    message(FATAL_ERROR "libsystemd development files are required")
endif ()

find_package(prometheus-cpp REQUIRED CONFIG)

add_executable(quill_syslog_sink_compile_test SyslogSinkCompileTest.cpp)
target_link_libraries(quill_syslog_sink_compile_test PRIVATE quill::quill)
set_common_compile_options(quill_syslog_sink_compile_test)

add_executable(quill_systemd_sink_compile_test SystemdSinkCompileTest.cpp)
target_include_directories(quill_systemd_sink_compile_test SYSTEM PRIVATE ${SYSTEMD_INCLUDE_DIR})
target_link_libraries(quill_systemd_sink_compile_test PRIVATE quill::quill ${SYSTEMD_LIBRARY})
set_common_compile_options(quill_systemd_sink_compile_test)

add_executable(quill_prometheus_sink_compile_test PrometheusSinkCompileTest.cpp)
target_link_libraries(quill_prometheus_sink_compile_test
        PRIVATE quill::quill prometheus-cpp::core prometheus-cpp::pull)
set_common_compile_options(quill_prometheus_sink_compile_test)
