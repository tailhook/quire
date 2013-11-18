==========
User Guide
==========


.. _cheat-sheet:

Yaml Cheat Sheet
================

Usually YAML structure is denoted by indentation.

.. _quire-tricks:

Quire Tricks
============


Underscore Names
----------------

.. _integers:

Integers
--------

Integers can be of base 10, just like everybody used to. It can also start
with ``0x`` to be interpreted as base 16, and if it starts with zero it is
interpreted as an octal number.

.. _units:

Units
-----

A lot of integer values in configuration files are quite big, e.g. should be
expressed in megabytes or gigabytes. Instead of common case of making default
units of megabytes or any other arbitrary choice, quire allows to specify
order of magnitude units for every integer and floating point value. E.g:

.. code-block:: yaml

    int1: 1M
    int2: 2k
    int3: 2ki

Results into the following, after parsing:

.. code-block:: yaml

   int1: 1000000
   int2: 2000
   int3: 2048

Note that there is a difference between prefixes for powers of 1024 and powers
of the 1000.

The following table summarizes all units supported:

===== ===================
Unit  Value
===== ===================
k     1000
ki    1024
M     1000000
Mi    1048576
G     1000000000
Gi    1073741824
T     1000000000000
Ti    1099511627776
P     1000000000000000
Pi    1125899906842624
E     1000000000000000000
Ei    1152921504606846976
===== ===================


.. _variables:


Variables
=========

YAML has a notion of anchors. You can anchor the node with ampersand ``&``,
and then alias it's value with star ``*``. Here is an example:

.. code-block:: yaml

   var1: &amp some_value
   var2: *amp

When encountering the code above, the parser sees:

.. code-block:: yaml

   var1: some_value
   var2: some_value

It's very powerful and very useful thing. You can even anchor entire hierarchy:

.. code-block:: yaml

   map1: &a
     key1: value1
     key2: value2
   map2: *a

Yields:

.. code-block:: yaml

   map1:
     key1: value1
     key2: value2
   map2:
     key1: value1
     key2: value2

This is powerful for keeping yourself from writing too much code. But it only
allows to substitute the whole yaml node. So there is more powerful scalar
expansion:

.. code-block:: yaml

   var1: &var some_value
   var2: $var

Note we replaced the aliasing using star ``*`` with dollar sign ``$``. This
doesn't look more powerful. But now we can override the value from the command
line::

    ./myprog -Dvar=another_value

Which yields:

.. code-block:: yaml

   var1: some_value
   var2: another_value

You can also substitute a part of the string:

.. code-block:: yaml

   _target: &target world
   var1: hello $target

Let's play with it a bit:

.. code-block:: console

    $ ./myprog -f test.yaml -P
    var1: hello world
    $ ./myprog -f test.yaml -Dtarget=foo -P
    var1: hello foo

There are two things interesting above:

1. Anchors and scalar variables are somewhat interchangable

2. Command-line variables override anchors. So latter may be used as default
   values

Note using underscored names for declaring variables. It's described in
:ref:`quire tricks<quire-tricks>`.

There is even more powerful form of variable expansion:

.. code-block:: yaml

   _n: &n 100
   int1: ${2*n}k

This leverages several features. Let's see the result:

.. code-block:: yaml

   int1: 200000

Few comments:

1. The ``${...}`` expands an expression not just single variable

2. The variable is referenced without dollar ``$`` inside the expression

3. The result of substitution is parsed using same rules as plain scalar, so
   may use :ref:`units <units>` as well.


Includes
========


Include Raw File Data
---------------------

Include Yaml
------------

Include Sequence of Yamls
-------------------------

Include Mapping From Set of Files
---------------------------------

.. _map-merge:

Merging Mappings
================

We use standard YAML way for merging_ mappings. It's achieved using ``<<`` key
and either mapping or a list of mappings for the value.

.. _merging: http://yaml.org/type/merge.html

The most useful merging is with aliases. Example:

.. code-block:: yaml

    fruits: &fruits
      apple: yes
      banana: yes
    food:
      bread: yes
      milk: yes
      <<: *fruits

Will be parsed as:

.. code-block:: yaml

   fruits:
     apple: yes
     banana: yes
   food:
     bread: yes
     milk: yes
     apple: yes
     banana: yes

.. _seq-merge:

Merging Sequences
=================


