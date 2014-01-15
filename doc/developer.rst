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

Unlike in C there is only one integer type in quire. And it's represented by
``long`` value in C.

The simplest config::

    val: !Int

If you supply scalar, is stands for the default value::

    val: !Int 10

The comprehensive specification for integer is something like the following:

.. code-block:: yaml

   val: !Int
     default: 1
     min: 0
     max: 10
     description: This value is something that is set in config
     example: 100
     command-line:
       names: [-v, --val-set]
       group: Options
       metavar: NUM
       descr: This option sets val
     command-line-incr:
       name: --incr
       group: Options
       descr: This option increments val
     command-line-decr:
       name: --decr
       group: Options
       descr: This option decrements val

The field in C structure look like the following:

.. code-block:: c

   long val;

The additinal keys represent minimum and maximum value for the integer:

.. code-block:: yaml

   val: !Int
     min: 0
     max: 10

Both values are inclusive. If user specifies bigger or smaller value either
in configuration file or on command-line, error is printed and configuration
rejected. If value overflows by using increments by command-line arguments
(see below), the value is simply adjusted to the maximum or minimum value as
appropriate.

The additional command-line actions:

.. code-block:: yaml

   command-line-incr: --incr
   command-line-decr: --decr

May be used to increment the value in the configuration. They are applied
after parsing the configuration file, and *set*-style options (regardless of
the order of the command-line options). Mostly useful for log-level or similar
things. The value printed using ``--config-print`` option includes all
incr/decr arguments applied.

All integer values support parsing different :ref:`bases <integers>` (e.g.
``0xA1`` for hexadecimal 161) and :ref:`units <units>` (e.g. ``1M`` for one
million)


Boolean Type
------------

The simplest boolean::

    val: !Bool

If you supply scalar, is stands for the default value::

    val: !Bool yes

The comprehensive specification for boolean is something like the following:

.. code-block:: yaml

   val: !Bool
     default: no
     description: This value is something that is set in config
     example: true
     command-line:
       names: [-v, --val-set]
       group: Options
       metavar: BOOL
       descr: This option sets val
     command-line-enable:
       name: --yes
       group: Options
       descr: This option sets val to true
     command-line-disable:
       name: --no
       group: Options
       descr: This option sets val to false

The field in C structure look like the following:

.. code-block:: c

   int val;

The value of ``val`` is always either ``0`` or ``1`` which stands for boolean
false and true respectively.

The additional command-line actions:

.. code-block:: yaml

   command-line-enable: --yes
   command-line-disable: --no

May be used to enable/disable the value in the configuration. They are applied
after parsing the configuration file, and after *set*-style options. If
multiple enable/disable options used, the last one wins. The value printed
using ``--config-print`` option includes all enable/disable arguments applied.

The following values may be used as booleans, both on the command-line and in
configuration file. The values are case insensitive:

============== =====
False          True
============== =====
false          true
no             yes
n              y
``~``
*empty string*
============== =====


Floating Point Type
-------------------

The simplest config::

    val: !Float

If you supply scalar, is stands for the default value::

    val: !Float 1.5

The comprehensive specification for floating point is something like the
following:

.. code-block:: yaml

   val: !Float
     default: 1.5
     description: This value is something that is set in config
     example: 2.5
     command-line:
       names: [-v, --val-set]
       group: Options
       metavar: FLOAT
       descr: This option sets val

The field in C structure look like the following:

.. code-block:: c

   double val;

All floating point values support parsing decimal numbers, optionally followed
by ``e`` and a decimal exponent. Floating point values also support
:ref:`units <units>` (e.g. ``1M`` for one million). Note that fractional units
are not supported yet.


Array Type
----------

The array type has no short form, and is always written as a mapping. The only
key required in the mapping is an ``element`` which denotes the type of item
in each array element.

.. code-block:: yaml

   arr: !Array
     element: !Int

Any quire type may be the element of the array. Including array itself. More
comprehensive example below:

.. code-block:: yaml

   arr: !Array
     description: Array of strings
     element: !String hello
     example: [hello, world]

.. note:: Command-line argument parsing is not supported neither for the array
   itself nor for any child of it. This may be improved in future. But look
   at :ref:`variables <variables>`, if you need some command-line customization.

The C structure for the array is a linked list:

.. code-block:: c

    struct cfg_a_str {
        struct cfg_a_str *next;
        const char *val;
        int val_len;
    };

    struct cfg_main {
        qu_config_head head;
        struct cfg_a_str *arr;
        struct cfg_a_str **arr_tail;
        int arr_len;
    };

The example of array usage is given in :ref:`tutorial <example-array>`.


Mapping Type
------------

The mapping type has no short form, and is always written as a mapping. The
two properties required in the mapping are ``key-element`` and
``value-element`` which denote the type of key and value for the mapping.

.. code-block:: yaml

   arr: !Mapping
     key-element: !Int
     value-element: !String

Any quire type may be the value element of the array. Including array itself.
A key may be any *scalar* type. More comprehensive example below:

.. code-block:: yaml

   map: !Mappings
     description: A mapping of string to structure
     key-element: !String
     value-element: !String
     example:
        apple: fruit
        carrot vegetable

.. note:: Command-line argument parsing is not supported neither for the
   mapping itself nor for any child of it. This may be improved in future. But
   look at :ref:`variables <variables>`, if you need some command-line
   customization.

The C structure for the mapping is a linked list:

.. code-block:: c

    struct cfg_m_str_str {
        struct cfg_m_str_str *next;
        const char *key;
        int key_len;
        const char *val;
        int val_len;
    };

    struct cfg_main {
        qu_config_head head;
        struct cfg_m_str_str *map;
        struct cfg_m_str_str **map_tail;
        int map_len;
    };

The example of mapping usage is given in :ref:`tutorial <example-mapping>`.


Custom Type
-----------

Sometimes you want to reuse a part of the config in multiple places. You
can do this with yaml aliases. But it's better to be done by declaring a
custom type. Here we will describe only how to refer to a custom type.
See :ref:`custom types <custom-types>` for a way to declare a type.

The simplest type reference is:

.. code-block:: yaml

   val: !Type type_name

As with most types, declaration may be expanded to a mapping:

.. code-block:: yaml

   val: !Type
     description: My Value
     type: type_name
     example: some data

.. note:: Neither command-line, nor default are supported for type reference
   for now. But this is expected to be improved in future


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


Include
-------

There is ``__include__`` special key, which allows to add ``#include``
directive to the generated configuration file header. This key can be present
at any place and will add the preprocessor directive at the top of the file.

For example:

.. code-block:: yaml

   __include__: "types.h"

Will result into the following line in the ``config.h`` file:

.. code-block:: c

   #include "types.h"

.. note:: There is no way to include a system header (``#include <filename>``),
   you can include some intermediate file, which includes the system header, if
   you really need the functionality. But most of the time double-quoted name
   will be searched for in system folders if not found in the project itself.


Set Flags
---------

The flag ``__set_flags__`` can be used to generate ``xx_set`` field for each
of the structure field. This flag may be used to find out whether field is set
by user or the default value is provided. For example:

.. code-block:: yaml

    data:
      ? __set_flags__
      a: !Int 1
      b: !Int 2

.. note:: The syntax ``? __set_flags__`` is YAML shortcut to
   ``__set_flags__: null``. We use and recommend this syntax for structure
   flags as it's not only shorter, but also stand out from structure field
   definitions.

Will turn into the following structure:

.. code-block:: c

   struct cfg_main {
     qu_config_head head;
     struct {
        unsigned int a_set:1;
        unsigned int b_set:1;
        long a;
        long b;
     } data;
   };

.. note:: The syntax ``int yy:1;`` is a syntax for bit field. I.e. the field
   that is only one bit in width. Given it is unsigned it can have
   one of the two values ``0`` and ``1``.

The values of ``a`` and ``b`` fields will always be intitialized (to 1 and 2
respectively), but the ``a_set`` and ``b_set`` will be non-zero only when
user specified them in configuration file.

The ``__set_flags__`` property can be specified in any structure, including the
root structure and ``!Struct`` custom type or its descendent. The flag is
propagated to the nested structures but not to the ``!Type`` fields.


Structure Name
--------------

Usually nested mappings that do not denoted by ``!Type`` are represented by
anonymous structures. But you can set ``__name__`` for the structure to have
a name.

.. code-block:: yaml

    data:
      __name__: data
      a: !Int 1
      b: !Int 2

Will name the internal structure:

.. code-block:: c

   struct cfg_main {
     qu_config_head head;
     struct cfg_data {
        unsigned int a_set:1;
        unsigned int b_set:1;
        long a;
        long b;
     } data;
   };

This is occasionally useful to use the structures in code.

.. note:: Author of config is responsible to set unique name of the structure
   otherwise the C compiler will throw an error.


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
========

.. warning::
   The functionality described in this section is currently discouraged and is
   subject to removing/adjusting at any time.

Ocasionally there is a need to put custom C field into generated structure.
You can do that with the following syntax:

.. code-block:: yaml

   _field-name: !CDecl struct some_c_struct

Where ``_field-name`` may be arbitrary but must start with underscore. And at
the right of the ``!CDecl`` may be any C type that compiler is able to
understand. It's written as is, so may potentially produce broken header if
some garbage is written instead of the type name.

If you need to add some header for type to be known to the compiler use
``__include__`` special key:

.. code-block:: yaml

    __include__: "types.h"
    _field-name: !CDecl struct some_c_struct

Note all files are added with ``#include "filename"`` syntax, *not* the
``#include <filename>``.

.. _YAML: http://yaml.org
.. _cmake: http://cmake.org
