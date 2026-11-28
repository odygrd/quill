.. title:: Installing

.. meta::
   :description: Install Quill for C++ with vcpkg, Conan, Homebrew, Conda, Meson, Bazel, CMake, or directly from source.

Installing
==========

Install Quill with a package manager, build it from source for a newer branch or local patches, or vendor it
inside your project. The optional C++20 module requires a source build.

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

Install from source to make Quill available to ``find_package()``.

.. code:: bash

   git clone https://github.com/odygrd/quill.git
   cd quill
   mkdir cmake_build
   cd cmake_build
   cmake ..
   make install

Set ``-DCMAKE_INSTALL_PREFIX=/quill/install-dir/`` for a custom install directory. Then use
``find_package(quill REQUIRED)`` in the consuming project.

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

For a vendored copy, add the source directory with ``add_subdirectory()``.

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

Using the Bundled Formatter
---------------------------

A formatting-only target can use the bundled formatter without inheriting Quill's logging compile options,
definitions or link dependencies.

.. md-tab-set::
   :class: quill-tabs

   .. md-tab-item:: CMake

      Link ``quill::fmtquill`` after ``find_package(quill REQUIRED)`` or ``add_subdirectory(quill)``.

      .. code:: cmake

         find_package(quill REQUIRED)
         target_link_libraries(my_project PRIVATE quill::fmtquill)

   .. md-tab-item:: Meson

      Use the installed ``fmtquill`` pkg-config dependency or fall back to a Quill subproject.

      .. code:: meson

         fmtquill_dep = dependency('fmtquill', fallback : ['quill', 'fmtquill_dep'])
         executable('my_project', 'main.cpp', dependencies : [fmtquill_dep])

   .. md-tab-item:: Bazel

      Add ``@quill//:fmtquill`` to the target's ``deps``.

Include ``quill/bundled/fmt/format.h`` for ``fmtquill::format`` and ``fmtquill::formatter`` specializations.
This header-only target uses Quill's patched {fmt} in namespace ``fmtquill``. Its version follows the Quill release.

Using the C++20 Module
----------------------

The experimental, opt-in named module lives in ``src/quill.cc``. Ordinary header-only use requires only C++17.

.. md-tab-set::
   :class: quill-tabs

   .. md-tab-item:: CMake

      Use CMake 3.28+ with a C++20 module-capable compiler and generator. Set ``CMAKE_CXX_STANDARD=20`` and
      ``QUILL_BUILD_MODULE=ON`` with ``add_subdirectory`` or ``FetchContent``, then link ``quill::quill_module``:

      .. code:: cmake

         target_link_libraries(my_project PRIVATE quill::quill_module)

      - The module target comes only from a source build, not from an installed ``find_package(quill)`` package.
      - With CMake 4.5+, ``QUILL_IMPORT_STD=ON`` enables experimental ``import std`` (default OFF).

   .. md-tab-item:: Bazel

      Use Bazel 9+, the declared ``rules_cc`` 0.2.22 and a module-capable toolchain. Add ``@quill//:quill_module``
      to ``deps`` and enable C++20 for the whole build with ``--experimental_cpp_modules`` (for Clang,
      ``--cxxopt=-std=c++20 --experimental_cpp_modules``):

      .. code:: python

         cc_binary(
             name = "my_project",
             srcs = ["main.cpp"],
             features = ["cpp_modules", "prefer_pic_for_opt_binaries"],
             deps = ["@quill//:quill_module"],
         )

      - ``prefer_pic_for_opt_binaries`` keeps module producers and consumers in PIC mode, avoiding
        conflicting module metadata outputs in optimized builds with ``rules_cc`` 0.2.22.
      - **import std:** point ``--@quill//:std_module`` at your toolchain's standard library module target,
        e.g. ``build --@quill//:std_module=@std_module`` in ``.bazelrc``. Quill then defines ``QUILL_IMPORT_STD``
        and adds the dependency. The default empty target falls back to standard library headers.
      - **Clang:** the toolchain needs a matching ``clang-scan-deps`` beside the compiler. On Ubuntu 24.04,
        install ``clang-18`` and ``clang-tools-18`` and set ``CC=/usr/lib/llvm-18/bin/clang`` and
        ``CXX=/usr/lib/llvm-18/bin/clang++``.
      - The module target is excluded from wildcard builds. Older Bazel versions can still use
        ``@quill//:quill`` and ``@quill//:fmtquill`` without enabling modules.

CMake and Bazel module targets define ``QUILL_USE_MODULE`` for consumers. Modules cannot export macros.
To use logging macros, include ``quill/LogMacros.h`` after the import. The guard below also handles custom builds.

.. literalinclude:: snippets/quill_docs_module.cpp
   :language: cpp

Mixing Module Imports and Headers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use either module imports or declaration headers consistently across the application and its libraries.
Mixing them is unsupported and can produce distinct Quill types, logger registries and backend state.

The module target depends on the header target for build requirements. That dependency is safe. With
``QUILL_USE_MODULE`` defined, you may include ``quill/LogMacros.h`` and ``quill/HelperMacros.h`` after import.

Next Steps
----------

.. rst-class:: quill-link-list

- :doc:`Quick Start <quick_start>` for a minimal working example.
- :doc:`Guides <guides>` for sinks, formatters, and advanced configuration.
- :doc:`Recipes <recipes>` for common tasks and code examples.
