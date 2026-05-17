find_package(Doxygen REQUIRED)

# Find all the public headers
set(QUILL_HEADER_DIR "${CMAKE_SOURCE_DIR}/include/quill")
file(GLOB_RECURSE QUILL_PUBLIC_HEADERS "${QUILL_HEADER_DIR}/*.h")

set(DOXYGEN_INPUT_DIR ${QUILL_HEADER_DIR})
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYGEN_TAG_FILE ${DOXYGEN_OUTPUT_DIR}/quill.tag)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

#Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) #Doxygen won't create this for us
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE} ${DOXYGEN_TAG_FILE}
        DEPENDS ${QUILL_PUBLIC_HEADERS}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
        COMMENT "Generating docs")

add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE} ${DOXYGEN_TAG_FILE})

find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)
set(SPHINX_TAG_FILE ${SPHINX_BUILD}/quill.tag)

# Pick up every docs source automatically so adding a new .rst does not silently
# skip the Sphinx rebuild trigger. CONFIGURE_DEPENDS makes CMake re-glob on build.
file(GLOB SPHINX_RST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.rst")

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Our doc files have been updated
# - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE} ${SPHINX_TAG_FILE}
        COMMAND
        ${SPHINX_EXECUTABLE} -b html
        # Tell Breathe where to find the Doxygen output
        -Dbreathe_projects.Quill=${DOXYGEN_OUTPUT_DIR}/xml
        ${SPHINX_SOURCE} ${SPHINX_BUILD}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DOXYGEN_TAG_FILE} ${SPHINX_TAG_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS
        ${SPHINX_RST_SOURCES}
        ${DOXYGEN_INDEX_FILE}
        ${DOXYGEN_TAG_FILE}
        MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
        COMMENT "Generating documentation with Sphinx")

# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE} ${SPHINX_TAG_FILE})
