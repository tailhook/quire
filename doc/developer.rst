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

    # Assuming you have "exe_name" executable
    # Add "config.c" that's will be generated to list of sources
    ADD_EXECUTABLE(exe_name
        main.c
        other.c
        config.c)
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
    # Add include search path for quire.h (overriding system one if exists)
    INCLUDE_DIRECTORIES(BEFORE SYSTEM quire/include)
    # Add linkage, adjust "exe_name" to name of your executable
    TARGET_LINK_LIBRARIES(exe_name quire)

Now just run ``cmake && make`` like you always do with cmake.

You also need to include folder ``quire`` to your source distributions, even
if they have C files generated. You also need to add instructions to run
``git submodule update --init`` for building from git.


.. _variable-types:

Variable Types
==============

All variable declarations start with yaml tag (an string starting with
exclamation mark). Almost any type can be declared in it's short form, as a
(tagged) scalar:

.. code-block:: yaml

   val1: !Int 0
   val2: !String hello
   val3: !Bool yes
   val4: !Float 1.5
   val5: !Type some_type

And any type can be written in equivalent long form as a mapping:

.. code-block:: yaml

   val1: !Int
     default: 0
   val2: !String
     default: hello
   val3: !Bool
     default: yes
   val4: !Float
     default: 1.5
   val5: !Type
     type: some_type

Using the latter form adds more features to the type definition. Next section
describes properties that can be used in any type, and following sections
describe each type in detail.


Common Properties
-----------------

The following properties can be used for any type, given the it's written in
it's long form (in form of mapping). Here is a list (string is for the sake of
example, any type could be used):

.. code-block:: yaml

   val: !String
     description: This value is something that is set in config
     default: nothing-relevant
     example: something-cool
     only-command-line: no
     command-line:
       names: [-v, --val-set]
       group: Options
       metavar: STR
       descr: This option sets val

Let's take a closer look.

.. code-block:: yaml

   val: !String
     description: This value is something that is set in config

The description is displayed in the output of ``--config-print`` and ``-PP``
command-line options. It's reformatted to the 80 characters in width, on
output. If set it's also used in command-line option description (``--help``)
if not overriden in ``command-line`` section.

.. code-block:: yaml

   val: !String
     default: nothing-relevant

Set's default value for the property. It should be the same type as the target
value.

.. code-block:: yaml

   val: !String
     example: something-cool

Set's the example value for the configuration variable. It's only output in
``--config-print=example`` and may be any piece of yaml. However it's
recommended to obey same structure as a target value, as it may be enforced in
the future. See description of ``--config-print`` for more information.

.. code-block:: yaml

   val: !String
     only-command-line: yes

This flag marks an option to be accepted from the command-line only. It is
neither parsed in yaml file, nor printed using ``--config-print``, but
otherwise it is placed in the same place in configuration structure and
respect same rules. If there is no ``command-line`` (see below) for this
option, then a member of the structure is generated and default is set
anyway.

The command-line may be specified in several ways. The simplest is:

.. code-block:: yaml

   val: !String
     command-line: -v

This adds single command-line option. Several options can be used too, mostly
useful for having short and long options, but may be used for aliases too:

.. code-block:: yaml

   val: !String
     command-line: [-v, --val]

And full command-line specification is a mapping. Each property in a mapping
is described in detail below.

.. code-block:: yaml

   val1: !String
     command-line:
       name: -v
       names: [-v, --val]

Either ``name`` or ``names`` may be specified, for the single option and
multiple options respectively.

.. code-block:: yaml

   val1: !String
     command-line:
       group: Options

The group of the options in the ``--help``. Doesn't have any semantic meaning
just keeps list of options nice. By default all options are listed under group
``Options``.

.. code-block:: yaml

   val1: !String
     command-line:
       metavar: STR

The metavar that's used in command-line description, e.g. ``--val STR``. By
default reasonably good type-specific name is used.

.. code-block:: yaml

   val1: !String
     command-line:
       descr: This option sets val

The description used in ``--help``. If not set, the ``description`` in the
option definition is used, if the latter is absent, some text similar to
``Set "val"`` is used instead.

There are also type-specific command-line actions:

.. code-block:: yaml

   intval: !Int
     command-line-incr: --incr
     command-line-decr: --decr
   boolval: !Bool
     command-line-enable: --enable
     command-line-disable: --disable

They all obey pattern ``command-line-ACTION``. Every such option may be
specified by any ways that ``command-line`` can. However, they have the
following difference:

* they inherit ``group`` from the ``command-line`` if specified
* they often have ``metavar`` useless
* they don't inherit ``description`` as it's usually misleading


String Type
-----------

String is most primitive data type. It accepts any YAML scalar and stores it's
value as ``const char *`` along with it's length.

The simplest config::

    val: !String

If you supply scalar, is stands for the default value::

    val: !String default_value

Maximum specification for string is something like the following:

.. code-block:: yaml

   val: !String
     description: This value is something that is set in config
     default: default_value
     example: some example
     command-line:
       names: [-v, --val-set]
       group: Options
       metavar: STR
       descr: This option sets val

The fields in C structure look like the following:

.. code-block:: c

   const char *val;
   int val_len;

Note that the string is both nul-terminated and has length in the structure.

.. warning::

   Technically it's possible that the string contain embedded nulls. In most
   cases this fact may be ignored. But do not rely on ``val_len`` be the length
   of the string after ``strdup`` or similar operation.



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



Special Keys
============


Types
-----

The ``__types__`` defines the custom types that can be used in multiple
places inside the configuration. It can also be used to define recursive types.
Any type defined inside ``__types__`` can be referred by
``!Type name_of_the_type``. See :ref:`custom types <custom-types>` for more info.


Conditionals
------------

There is a common use case where you have several utilities sharing mostly
same config with some deviations. The most typical use case is a daemon
process and a command-line interface to it, with a different set of
command-line argumemnts. Here is how it looks like:

.. code-block:: yaml

   __if__:defined CLIENT:
    query: !String
      only-command-line: yes
      command-line: --query

When compiling utility you should *define* the ``CLIENT`` macro::

    gcc ... -DCLIENT

And you will get additional command-line arguments for this binary. In code
it looks like:

.. code-block:: c

   struct cfg_main_t {
       int val1;
   #if defined CLIENT
       const char *query;
       int query_len;
   #endif  /* defined CLIENT */
   }

The rule is: if expression is evaluated to true, you get the configuration with
all the contents of conditional merged inside the mapping (i.e. conditional
replaced by ``<<:``). In case expression is evaluated to false, you should get
the all the configuration structures and semantics as the key and all its
contents doesn't exist at all.

You can use any expression that C preprocessor is able to evaluate instead of
``defined CLIENT``

.. warning::
   You must define the macro consistently across all C files that use
   configuration header (``config.h``). In particular you can't share
   ``config.o`` generated for the two executables having different definitions.
   CMake handles this case automatically but some other build systems don't.



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

C Fields
--------

Ocasionally there is a need to put custom C field into generated structure.
You can do that with the following syntax:

.. code-block:: yaml

   _field-name: !CDecl struct some_c_struct

Where ``_field-name`` may be arbitrary but must start with underscore. And at
the right of the ``!CDecl`` may be any C type that compiler is able to
understand. It's output as is so may potentially produce broken header if some
garbage is written instead of the type info.

.. warning::
   This functionality is currently discouraged, and may be removed/adjusted in
   future releases.

.. _YAML: http://yaml.org
.. _cmake: http://cmake.org
