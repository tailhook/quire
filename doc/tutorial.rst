==============
Quire Tutorial
==============

This tutorial assumes that reader is proficient with C and build tools,
and have setup building as described in :ref:`Developer Guide <build-process>`.

It is also assumed that you are familiar with YAML_. There is quick
:ref:`cheat sheet <cheat-sheet>` in user guide.


.. _minimal-config:

Minimal Config
==============

Let's make minimal configuration definition file, which is by coincidence is
also a YAML file. By convention it's called ``config.yaml``::

    __meta__:
      program-name: frog
      default-config: /etc/frog.yaml
      description: The test program that is called frog just because it
        rhymes with prog (i.e. abbreviated "program")

Now let's build them to see what we have (we use ``quire-tool`` like it's
installed in the system, you can use any :ref:`build tool <build-process>`
to build the files)::

    quire-tool --source config.yaml --c-header config.h --c-header config.c

Let's take a look at what we have in ``config.h``:

.. code-block:: c

    /*  Main configuration structure  */
    struct cfg_main {
        qu_config_head head;
    };

    /*  API  */
    int cfg_load(struct cfg_main *cfg, int argc, char **argv);
    void cfg_free(struct cfg_main *cfg);

.. admonition:: Disclaimer

   Generated code pieces are shown stripped and reformatted for teaching
   purposes


We don't see anything useful here yet. But let's make it work anyway. We need
a ``main.c``:

.. code-block:: c

    #include "config.h"

    static void run_program(struct cfg_main *cfg) {
        printf("The test program is doing nothing right now!\n");
    }

    int main(int argc, char **argv) {
        int rc;
        struct cfg_main cfg;

        rc = cfg_load(&cfg, argc, argv);
        if(rc == 0) {
            run_program(&cfg);
        }
        cfg_free(&cfg);

        if(rc > 0) {
            /*  rc > 0 means we had some configuration error  */
            return rc;
        } else {
            /*  rc == 0 means we have run successfully  */
            /*  rc < 0 means we've done some config action successfully  */
            return 0;
        }
    }

As you can see there is a tiny bit of boilerplate with handling error codes
and freeing memory. Let's build it::

    gcc main.c config.c -o frog -lquire -g

Let's see what batteries we have out of the box:

.. code-block:: console

    $ ./prog
    Error parsing file /etc/frog.yaml: No such file or directory

Hm, we don't have a configuration file, yet. And we don't want to put
configuration into ``/etc`` yet. Let's see what we can do:

.. code-block:: console

    $ ./prog --help
    Usage:
        frog [-c CONFIG_PATH] [options]

    The test program that is called frog just because it rhymes with prog (i.e.
    abbreviated "program")

    Configuration Options:
      -h,--help         Print this help
      -c,--config PATH  Configuration file name [default: /etc/frog.yaml]
      -D,--config-var NAME=VALUE
                        Set value of configuration variable NAME to VALUE
      -C,--config-check
                        Check configuration and exit
      -P                Print configuration after reading, then exit. The
                        configuration printed by this option includes values
                        overriden from command-line. Double flag `-PP` prints
                        comments.
      --config-print TYPE
                        Print configuration file after reading. TYPE maybe
                        "current", "details", "example", "all", "full"

You can change path to configuration file, you can play with configuration
checking and printing, you can put some variables into configuration (more
below). And you get all of this for free.

So to run the command now, execute:

.. code-block:: console

   $ touch frog.yaml
   $ ./frog -c frog.yaml
   The test program is doing nothing right now!

Let's make it easier to test by picking up configuration file from current
directory:

.. code-block:: yaml

    __meta__:
      ...
      default-config: frog.yaml
      ...

.. code-block:: console

   $ ./frog
   The test program is doing nothing right now!


Adding Useful Stuff
===================

Let's add some integer knob to our config:

.. code-block:: yaml

   jumps: !Int 3

After building we have the following header:

.. code-block:: c

   struct cfg_main {
       qu_config_head head;
       long jumps;
   };

And we can now make advantage of this variable:

.. code-block:: c

    void run_program(struct cfg_main *cfg) {
        int i;
        for(i = 0; i < cfg->jumps; ++i) {
            printf("jump\n");
        }
    }

Let's run and play with it a little bit:

.. code-block:: console

   $ ./frog
   jump
   jump
   jump
   $ echo "jumps: 4" > frog.yaml
   $ ./frog
   jump
   jump
   jump
   jump

Note: I'm editing the file by shell command. It's probably too freaky way to
do that. You can just edit the file, and see how changes are reflected.

The tutorial gives you an overview of what quire is able to parse and generate,
for full list of types supported see :ref:`Developer Guide <variable-types>`.


Nested Structures
=================

Now the interesting begins. You can make hierarchical config, configuration
sections of arbitrary depth:

.. code-block:: yaml

   jumping:
     number: !Int 3
     distance: !Float 1

Yields:

.. code-block:: c

   struct cfg_main {
       qu_config_head head;
       struct {
           long number;
           double distance;
       } jumping;
   };

In config it looks like:

.. code-block:: yaml

   jumping:
     number: 5
     distance: 2

.. note::

   The presence of nested structures in quire doesn't mean that nesting too
   deep is encouraged. Probably the example above is better written as:

   .. code-block:: yaml

      jumping-number: !Int 3
      jumping-distance: !Float 1

   Particularly, flat structure is more convenient for
   :ref:`merging <map-merge>` maps. So use nested structures sparingly.


Command-line Arguments
======================

Many values can be controlled from the command-line. Let's return to the
simpler example:

.. code-block:: yaml

   jumps: !Int 3

Command-line is enabled easily. First we should reformat our declaration, to
equivalent one with mapping syntax:

.. code-block:: yaml

   jumps: !Int
     default: 3

Now we can add a command-line option:

.. code-block:: yaml

   jumps: !Int
     default: 3
     command-line: [-j, --jumps]

Let's see:

.. code-block:: console

   $ ./frog --help
   Usage:
       frog [-c CONFIG_PATH] [options]

   The test program that is called frog just because it rhymes with prog (i.e.
   abbreviated "program")

   Configuration Options:
     -h,--help         Print this help
     -c,--config PATH  Configuration file name [default: /etc/frog.yaml]
     -D,--config-var NAME=VALUE
                       Set value of configuration variable NAME to VALUE
     -C,--config-check
                       Check configuration and exit
     -P                Print configuration after reading, then exit. The
                       configuration printed by this option includes values
                       overriden from command-line. Double flag `-PP` prints
                       comments.
     --config-print TYPE
                       Print configuration file after reading. TYPE maybe
                       "current", "details", "example", "all", "full"

   Options:
     -j,--jumps INT    Set "jumps"
   $ ./frog
   jump
   jump
   jump
   $ ./frog -j 1
   jump
   $ ./frog --jumps=2
   jump
   jump
   $ ./frog --ju 1
   jump

For integer types there are increment and decrement arguments:

.. code-block:: yaml

   jumps: !Int
     default: 3
     command-line: [-j, --jumps]
     command-line-incr: --jump-incr
     command-line-decr: [-J,--jump-decr]

This works as following:

.. code-block:: console

   $ ./frog
   jump
   jump
   jump
   $ ./frog --jump-decr
   jump
   jump
   $ ./frog -JJ
   jump
   $ ./frog -JJJ
   $ ./frog --jump
   Option error "--jump": Ambiguous option abbreviation

.. note::

   Making command-line arguments is easy. However, too many command-line
   options makes ``--help`` output too long. There is another mechanism to
   expose configuration variables to the command-line:
   :ref:`variables <variables>`. Variables in quire are even more powerful, but
   somewhat less easy to use. At the end of the day, declare command-line
   arguments for options that either useful for almost every user, or
   should only be specified in the command-line.

.. _example-array:

Arrays
======

So far we have only declared simple options, that every configuration library,
supports. But here is where the power of the quire comes. The arrays are
declared like the following:

.. code-block:: yaml

   sounds: !Array
     element: !String

Here we declared array of strings. Here is how it looks like in C structure:

.. code-block:: c

    struct cfg_a_str {
        struct cfg_a_str *next;
        const char *val;
        int val_len;
    };

    struct cfg_main {
        qu_config_head head;
        struct cfg_a_str *sounds;
        struct cfg_a_str **sounds_tail;
        int sounds_len;
    };

It's looks too ugly at the first glance. But the rules are:

1. The array is a linked list
2. The type of list element is named ``cfg_a_TYPENAME``
3. The head of the linked list is named as variable in yaml
4. The tail may be ignored unless you want to insert another element
5. There is ``_len``-suffixed element for the number of elements in array
6. The element of linked list is named ``val`` (suffixes work here too)

Ok, let's see how to use it in code:

.. code-block:: c

   struct cfg_a_str *el;
   for(el = cfg->sounds; el; el = el->next) {
       printf("%s\n", el->val);
   }

Now if we write following config:

.. code-block:: yaml

   sounds:
   - croak
   - ribbit

We can have a frog that can cry with both USA and UK slang :)

.. code-block:: console

    $ ./frog -c flog.yaml
    croak
    ribbit

You can also create nested arrays, and arrays of structures.

.. _example-mapping:

Mappings
========

We can also declare a mapping:

.. code-block:: yaml

   sounds: !Mapping
     key-element: !String
     value-element: !String

Here we declared mapping of string to string. Here is how it looks like in C
structure:

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
        struct cfg_m_str_str *sounds;
        struct cfg_m_str_str **sounds_tail;
        int sounds_len;
    };

The structure is very similar to array's one, but the element type is named
``cfg_m_KEYTYPE_VALUETYPE``.

Ok, let's see how to use it in code:

.. code-block:: c

   struct cfg_a_str *el;
   for(el = cfg->sounds; el; el = el->next) {
       printf("%s -- %s\n", el->key, el->val);
   }

Now if we write following config:

.. code-block:: yaml

   sounds:
    gb: croak
    usa: ribbit

We can have a frog that can display both the slang and the text:

.. code-block:: console

    $ ./frog -c flog.yaml
    gb -- croak
    usa -- ribbit

.. note::
   The mapping is represented by a linked list too. There is no hash table or
   other mapping structures that makes access by key fast. There are few
   reasons for this decision, the most imporant one is that most programs will
   copy the mapping into their own hash table implementation anyway.


.. warning::
   The order of the elements in the linked list is preserved. But this
   shouldn't be relied upon, as the YAML spec doesn't guarantee that.
   For example some tool may rewrite yaml file and get keys reordered.


The ``key-element`` can be any scalar type (string, int, float...).

The ``value-element`` can be any type supported by quire, including nested
arrays and mappings.

Custom Types
============



.. _YAML: http://yaml.org
