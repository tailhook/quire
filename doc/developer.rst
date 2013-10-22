===============
Developer Guide
===============


.. _build-process:

Build Process
=============

This section discusses how to run quire-gen and how to use quire as the
part of your applications, using different build systems.

.. note::
   The ABI for the library is not stable so the recommended way of using
   quire is by using it as ``git submodule`` of your application.


Raw Process
-----------

Whole configuration parser generation is based on single YAML_ file.
By convention it's called "config.yaml" and is put near the sources of the
project (e.g. "src/" folder).

If you have installed quire to system, to make parser generator run:

    quire-gen --source config.yaml --c-header config.h --c-header config.c

Then you may use the files as normal C sources. But be careful to update them
when yaml file changes. If you add them as a part of build process you may need
the mark as "generated" or equal, so that build system would not error if they
are absent. See below for instructions for specific build systems. You do *not*
need to bundle original yaml file with distribution of your application.

This is it. See :ref:`tutorial <minimal-config>` for examples of the yaml
itself and how to use it in your own code.


Using Make
----------

To use make for configuration file generation you might write something
along the lines of::

    config.h config.c: config.yaml
        quire-gen --source $^ --c-header config.h --c-source config.c


Using CMake and Git Submodule
-----------------------------

If you are using cmake_ for building your project, you are lucky, because the
developers of quire use cmake too. So the whole process is easy.

Let's add submodule first::

    git submodule add git@github.com:tailhook/quire quire

Now we should add the following to the ``CMakeLists.txt``:

.. code-block:: cmake

    # Builds quire itself
    ADD_SUBDIRECTORY(quire)
    # Get's the full path of quire-gen executable just built
    GET_TARGET_PROPERTY(QUIRE_GEN quire-gen LOCATION)
    # Adds target to build C files and headers
    # You may need to adjust source and/or directory
    ADD_CUSTOM_COMMAND(
        OUTPUT config.h config.c
        COMMAND ${QUIRE_GEN}
            --source ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
            --c-header config.h
            --c-source config.c
        DEPENDS config.yaml quire-gen
        )
    # Marks files as generated so make/cmake doesn't complain they are absent
    SET_SOURCE_FILES_PROPERTIES(
        config.h config.c
        PROPERTIES GENERATED 1
        )
    # Add include search path for the files that include "config.h"
    SET_SOURCE_FILES_PROPERTIES(
        main.c other.c  # These files need to be adjusted
        COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}")
    # Add linkage, adjust "exe_name" to name of your executable
    TARGET_LINK_LIBRARIES(exe_name quire)
    # Add "config.c" that's just generated to list of sources for your executable
    ADD_EXECUTABLE(exe_name
        main.c
        other.c
        config.c)

Now just run ``cmake && make`` like you always do with cmake.

You also need to include folder ``quire`` to your source distributions, even
if they have C files generated. You also need to add instructions to run
``git submodule update --init`` for building from git.


.. _variable-types:

Variable Types
==============

String Type
-----------

Integer Type
------------

Boolean Type
------------

Floating Point Type
-------------------

Array Type
----------

Mapping Type
------------

Custom Type
-----------

.. _custom-types:

Custom Types
============


Structure Type
--------------

Choice Type
-----------

Enumeration Type
----------------

Tagged Scalar Type
------------------

.. _YAML: http://yaml.org
.. _cmake: http://cmake.org
