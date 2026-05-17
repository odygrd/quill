.. title:: Installing

Installing
==========

For most users, a package manager is the simplest path. Build from source when you
want the latest branch, need local patches, or prefer to embed Quill directly in
your own project.

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

Use this path when you want a normal install that can later be found with ``find_package()``.

.. code:: bash

   git clone https://github.com/odygrd/quill.git
   cd quill
   mkdir cmake_build
   cd cmake_build
   cmake ..
   make install

.. note::

   To install into a custom directory, configure CMake with
   ``-DCMAKE_INSTALL_PREFIX=/quill/install-dir/``.

Then use the installed library from a CMake project, you can locate it directly with ``find_package()``

Embedded Project Directory Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   my_project/
   ├── CMakeLists.txt
   ├── main.cpp

Embedded Project CMakeLists.txt
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: cmake

   # Set only if needed - quill was installed under a custom non-standard directory
   set(CMAKE_PREFIX_PATH /test_quill/usr/local/)

   find_package(quill REQUIRED)

   # Linking your project against quill
   add_executable(example main.cpp)
   target_link_libraries(example PRIVATE quill::quill)

Embedding in Your Project
~~~~~~~~~~~~~~~~~~~~~~~~~

Use this path when you vendor Quill directly into your source tree and include it with
``add_subdirectory()``.

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

Next Steps
----------

- :doc:`Quick Start <quick_start>` for a minimal working example.
- :doc:`Guides <guides>` for sinks, formatters, and advanced configuration.
- :doc:`Recipes <recipes>` for common tasks and code examples.
