add_subdirectory(bundled)

# library name
set(TARGET_NAME quill)

# header files
set(HEADER_FILES
    include/quill/detail/misc/Attributes.h
    include/quill/detail/misc/Common.h
    include/quill/detail/misc/Macros.h
    include/quill/detail/misc/Os.h
    include/quill/detail/misc/RdtscClock.h
    include/quill/detail/misc/RecursiveSpinlock.h
    include/quill/detail/misc/Spinlock.h
    include/quill/detail/misc/Utilities.h
    include/quill/detail/misc/TypeTraits.h
    include/quill/detail/record/CommandRecord.h
    include/quill/detail/record/LogRecord.h
    include/quill/detail/record/RecordBase.h
    include/quill/detail/record/StaticLogRecordInfo.h
    include/quill/detail/BackendWorker.h
    include/quill/detail/BoundedSPSCQueue.h
    include/quill/detail/Config.h
    include/quill/detail/HandlerCollection.h
    include/quill/detail/LoggerCollection.h
    include/quill/detail/LoggerDetails.h
    include/quill/detail/LogMacros.h
    include/quill/detail/LogManager.h
    include/quill/detail/LogManagerSingleton.h
    include/quill/detail/ThreadContext.h
    include/quill/detail/ThreadContextCollection.h
    include/quill/detail/UnboundedSPSCQueue.h
    include/quill/handlers/FileHandler.h
    include/quill/handlers/Handler.h
    include/quill/handlers/StreamHandler.h
    include/quill/Logger.h
    include/quill/LogLevel.h
    include/quill/PatternFormatter.h
    include/quill/Quill.h
    include/quill/TweakMe.h
    include/quill/Utility.h
    include/quill/Version.h
    )

# source files
set(SOURCE_FILES
    src/detail/BackendWorker.cpp
    src/detail/Config.cpp
    src/detail/HandlerCollection.cpp
    src/detail/LoggerCollection.cpp
    src/detail/LogManager.cpp
    src/detail/LogManagerSingleton.cpp
    src/detail/ThreadContextCollection.cpp
    src/handlers/FileHandler.cpp
    src/handlers/StreamHandler.cpp
    src/utility/Utilities.cpp
    src/utility/Os.cpp
    src/utility/RdtscClock.cpp
    src/utility/RecursiveSpinlock.cpp
    src/utility/Spinlock.cpp
    src/LogLevel.cpp
    src/PatternFormatter.cpp
    src/Quill.cpp
    src/Utility.cpp
    )

# Add as a library
if (QUILL_BUILD_SHARED)
    add_library(${TARGET_NAME} SHARED "")
else ()
    add_library(${TARGET_NAME} STATIC "")
endif ()

add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

# Add target sources
target_sources(${TARGET_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

# Add include directories for this library
target_include_directories(${TARGET_NAME}
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
                           PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR})

# Link dependencies
target_link_libraries(${TARGET_NAME} PUBLIC fmt invoke Threads::Threads)

# Compiler options
target_compile_options(${TARGET_NAME} PRIVATE
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                       -Wall -Wextra -Wconversion -pedantic -Wfatal-errors -Wno-unused-private-field>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)

if (MSVC_VERSION GREATER 1900)  #Allow non fatal security warnings for msvc 2015
    target_compile_options(${TARGET_NAME} PRIVATE /WX)
endif ()

# TODO:: use MultiByteToWideChar() and WideCharToMultiByte() from <Windows.h> instead of covecvt
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${TARGET_NAME} PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif ()

# Properties
set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${QUILL_VERSION} SOVERSION ${QUILL_VERSION})

# ---- Tests ---- #
if (QUILL_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_subdirectory(test)
endif ()