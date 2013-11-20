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


Templates
=========

Even more powerful construction in a combination with variables is a template.
Template is basically an anchored node which has some variable references, and
may be used with different variable values in different contexts. For example:

.. code-block:: yaml

   _tpl: &price !NoVars
     chair: $x dollars
     table: ${x*4} dollars

   shops:
     cheap: !Template:price
       x: 50
     expensive: !Template:price
       x: 150

The example above will be expanded as the following:

.. code-block:: yaml

    shops:
      cheap:
        chair: 50 dollars
        table: 200 dollars
      expensive:
        chair: 150 dollars
        table: 600 dollars

The templates may be arbitrarily complex. There are few limitations:

1. Template-scoped variables may only be scalar

2. The anchored node is expanded too, you may either use ``!NoVars`` like in
   example, or define all the variables to get rid of warnings of
   ``Undefined variable``

3. Variables in mapping keys or tags are not supported

Note, the limitation #1, doesn't limit you to use anchor or templates inside
a template (the anchored node), just the scoped variables inside the template
invocation (the items of a mapping tagged ``!Template``) must be scalar. And
anchors are never scoped.


Includes
========

All includes have common structure. They are denoted by tagged scalar, with the
special tag. With the scalar being the path/filename to include. After parsing
the yaml but before converting the data into configuration file structure, the
node is replaced by the actual contents of the file(s).

Few more properties that are common for all include directives:

* All paths are relative to the configuration file name which contains the
  include directive (in fact relative to the name under which file is opened
  in case it symlinked into multiple places)

* Include directives can be arbirarily nested (up to the memory limit)

* File inclusion is logical not textual, so (a) each file must be a full valid
  YAML file (with the anchors exception described below), and (b) the included
  data is contained at the place where directive is (unlike many other
  configuration systems where inclusion usually occurs at the top level of the
  config), but you can include at the top level of the config too

* Variable references are not parsed in include file names yet, but it's on
  todo list, so do not rely on include paths that contain dollar signs

* There is a common namespace for anchors and variables between parent and
  include files, but this behavior may be changed in future


Include Raw File Data
---------------------

The ``!FromFile`` tag includes the contents of the file as a scalar value.
For example if ``somefile.txt`` has the following contents::

    line1
    : line2

The following yaml:

.. code-block:: yaml

   text: !FromFile "somefile.txt"

Is equivalent to:

.. code-block:: yaml

   text: "line1\n: line2"

The context of the file is not parsed. And it's the only way to include binary
data in configuration at the moment.


Include Yaml
------------

The ``!Include`` tag includes the contents of the file replaceing the
node that contains tag. For example:

.. code-block:: yaml

    # config.yaml
    items: !Include items.yaml

.. code-block:: yaml

    # items.yaml
    - apple
    - cherry
    - banana

Is equivalent of:

.. code-block:: yaml

   items:
   - apple
   - cherry
   - banana


Include Sequence of Yamls
-------------------------

The ``!GlobSeq`` tag includes the files matching a glob-like pattern, so that
each file represents an entry in the sequence. Each included file is a valid
YAML file.

The pattern is not full glob pattern (yet). It may contain only a single star
and an arbitrary prefix and suffix.

For example:

.. code-block:: yaml

    # config.yaml
    items: !GlobSeq fruits/*.yaml

.. code-block:: yaml

    # fruits/apple.yaml
    name: apple
    price: 1

.. code-block:: yaml

    # fruits/pear.yaml
    name: pear
    price: 2

Is equivalent of:

.. code-block:: yaml

   items:
   - name: apple
     price: 1
   - name: pear
     price: 2

.. note:: The entries are unsorted, so you should not use the ``!GlobSeq`` in
   places sensitive to positions for items. You should use plain sequence
   with ``!Include`` for each item instead

This construction is particularly powerful with :ref:`merge key<map-merge>`
``<<``. For example:

.. code-block:: yaml

    # config.yaml
    <<: !GlobSeq config/*.yaml

.. code-block:: yaml

    # config/basics.yaml
    firstname: John
    lastname: Smith

.. code-block:: yaml

    # config/location.yaml
    country: UK
    city: London

Is equivalent of:

.. code-block:: yaml

    firstname: John
    lastname: Smith
    country: UK
    city: London

Multiple sets of files might be concatenated using
:ref:`unpack operator<seq-merge>`.


Include Mapping From Set of Files
---------------------------------

The ``!GlobMap`` tag includes the files matching a glob-like pattern, so that
each file represents an entry in the mapping. The key in the mapping is
extracted from the part of the filename that is enclosed in parenthesis. Each
included file is a valid YAML file.

The pattern is not full glob pattern (yet). It may contain only a single star
and an arbitrary prefix and suffix. It must contain parenthesis and the star
character must be between the parenthesis.

.. code-block:: yaml

    # config.yaml
    items: !GlobSeq fruits/(*).yaml

.. code-block:: yaml

    # fruits/apple.yaml
    title: Russian Apples
    price: 1

.. code-block:: yaml

    # fruits/pear.yaml
    title: Sweet Pears
    price: 2

Is equivalent of:

.. code-block:: yaml

   items:
     apple:
       title: Russian Apples
       price: 1
     pear:
       title: Sweet Pears
       price: 2

You can also merge mappings from the multiple directories and do other crazy
things using :ref:`merge operator<map-merge>` ``<<``.

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


