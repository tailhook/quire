==============
Quire Tutorial
==============

This tutorial assumes that reader is proficient with C and build tools,
and have setup building as described in :ref:`Developer Guide <build-process>`.

It is also assumed that you are familiar with YAML_. There is quick
:ref:`cheat sheet <cheat-sheet>` in user guide.


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

Let's take a look at what we have in ``config.h``::

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
a ``main.c``::

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

    gcc main.c config.c -o frog -lquire

Let's see what batteries we have out of the box::

    $ ./prog
    Error parsing file /etc/frog.yaml: No such file or directory

Hm, we don't have a configuration file, yet. And we don't want to put
configuration into ``/etc`` yet. Let's see what we can do::

    $ ./prog --help
    Usage:
        frog [-c CONFIG_PATH] [options]

    The test program that is called frog just because it
     rhymes with prog (i.e.
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


.. _YAML: http://yaml.org
