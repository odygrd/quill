.. _install:

##############################################################################
Install
##############################################################################

Package Managers
=================

====================== ======================= ===================
Homebrew               vcpkg                   Conan
====================== ======================= ===================
``brew install quill`` ``vcpkg install quill`` ``quill/[>=1.2.3]``
====================== ======================= ===================

CMake-Integration
=================

External
--------

Building and Installing Quill as Static Library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

   git clone https://github.com/odygrd/quill.git
   mkdir cmake_build
   cd cmake_build
   make install

Note: To install in custom directory invoke cmake with ``-DCMAKE_INSTALL_PREFIX=/quill/install-dir/``

Building and Installing Quill as Static Library With External ``libfmt``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: bash

    cmake -DCMAKE_PREFIX_PATH=/my/fmt/fmt-config.cmake-directory/ -DQUILL_FMT_EXTERNAL=ON -DCMAKE_INSTALL_PREFIX=/quill/install-dir/'

Then use the library from a CMake project, you can locate it directly with ``find_package()``

Directory Structure
~~~~~~~~~~~~~~~~~~~

::

   my_project/
   ├── CMakeLists.txt
   ├── main.cpp

CMakeLists.txt
~~~~~~~~~~~~~~

.. code:: cmake

   # Set only if needed - quill was installed under a custom non-standard directory
   set(CMAKE_PREFIX_PATH /test_quill/usr/local/)

   find_package(quill REQUIRED)

   # Linking your project against quill
   add_executable(example main.cpp)
   target_link_libraries(example PRIVATE quill::quill)

Embedded
--------

To embed the library directly, copy the source to your project and call ``add_subdirectory()`` in your ``CMakeLists.txt`` file

Directory Structure
~~~~~~~~~~~~~~~~~~~

::

   my_project/
   ├── quill/            (source folder)
   ├── CMakeLists.txt
   ├── main.cpp

CMakeLists.txt
~~~~~~~~~~~~~~

.. code:: cmake

   add_subdirectory(quill)
   add_executable(my_project main.cpp)
   target_link_libraries(my_project PRIVATE quill::quill)
