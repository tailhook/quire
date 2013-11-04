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

Integers
--------

Integers can be of base 10, just like everybody used to. It can also start
with ``0x`` to be interpreted as base 16, and if it starts with zero it is
interpreted as an octal number.

.. _units:

Units
-----



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

We use standard YAML way for merging_ mapppings.

.. _merging: http://yaml.org/type/merge.html


.. _seq-merge:

Merging Sequences
=================


