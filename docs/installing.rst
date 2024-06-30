.. title:: Installing

Installing
==========

Package Managers
----------------

=================  ============================================
Package Manager    Installation Command
=================  ============================================
vcpkg              ``vcpkg install quill``
Conan              ``conan install quill``
Homebrew           ``brew install quill``
Meson WrapDB       ``meson wrap install quill``
Conda              ``conda install -c conda-forge quill``
Bzlmod             ``bazel_dep(name = "quill", version = "x.y.z")``
xmake              ``xrepo install quill``
=================  ============================================

CMake-Integration
-----------------

Building and Installing from Source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   git clone https://github.com/odygrd/quill.git
   mkdir cmake_build
   cd cmake_build
   cmake ..
   make install

Note: To install in custom directory invoke cmake with

``-DCMAKE_INSTALL_PREFIX=/quill/install-dir/``

Then use the installed library from a CMake project, you can locate it directly with ``find_package()``

Directory Structure
^^^^^^^^^^^^^^^^^^^

::

   my_project/
   ├── CMakeLists.txt
   ├── main.cpp

CMakeLists.txt
^^^^^^^^^^^^^^

.. code:: cmake

   # Set only if needed - quill was installed under a custom non-standard directory
   set(CMAKE_PREFIX_PATH /test_quill/usr/local/)

   find_package(quill REQUIRED)

   # Linking your project against quill
   add_executable(example main.cpp)
   target_link_libraries(example PRIVATE quill::quill)

Embedding in Your Project
~~~~~~~~~~~~~~~~~~~~~~~~~

To embed the library directly, copy its source code into your project directory and include it using
``add_subdirectory()`` in your ``CMakeLists.txt`` file.

Directory Structure
^^^^^^^^^^^^^^^^^^^

::

   my_project/
   ├── quill/            (source folder)
   ├── CMakeLists.txt
   ├── main.cpp

CMakeLists.txt
^^^^^^^^^^^^^^

.. code:: cmake

   add_subdirectory(quill)
   add_executable(my_project main.cpp)
   target_link_libraries(my_project PRIVATE quill::quill)