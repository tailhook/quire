quire
-----

quire is a parser generator for configuration files.

Parsing configuration files is uneasy work. There are a lot of configuration
file parsers, but syntax of most of them is ugly. Also all parsers I have seen
before, require you fetch every option from configuration library and copy it to
your own structure. Also it's not easy to make reusable parts for configuration
file (they have only scalar variables usually). So...

* We use YAML_ for configuration files
* We generate C structure, already typed appropriately, for your config
* Command-line parser is generated automatically (command-line overrides
  configuration file options)
* We use YAML config, to configure parser itself. So you decide the
  types of options, constraints, and command-line argument names
* Code, to print runtime configuration (including command-line overrides)
  is also made
* YAML has rich set of data types (arrays, mappings) which we use to make
  config useful (compare it with old ``.ini`` files)
* We use YAML-builtin anchors to make reusable parts
* We have extended YAML with runtime variables, substring substitution
  (and we some simple mathematical expressions too)

All this to meet the following goals:

 * Embed configuration in your own application with ease
 * Add each option only in one place
 * Document it just as easy as adding

.. _YAML: http://yaml.org


Build Instructions
------------------

Build process is done with cmake::

    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make
    make install

