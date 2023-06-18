# Regular Expressions to Finite-state Automaton (RE2FA) #

## Overview ##

The RE2FA is a small library that can generate Deterministic Finite-state
Automata (DFA) from a set of Regular Expression.
It could be used for researches of the Exponential Blowup problem.

Doxygen documentation is hosted at
[GitHub Pages](https://d06alexandrov.github.io/re2fa).

## Prerequisites ##

To build this library and corresponding tools you need the following software.

* Autotools including Automake and Libtool.
* (optional) libz library for DFA file compression.
* (for OSX) argp library.

## Building ##

To build this library and corresponding application you have to execute
the following commands from the top directory of the project.

Create configure script:
```
$ autoreconf -i
```

Generate makefiles:
```
$ ./configure
```

Build:
```
$ make
```

## Running application re2fa ##

Some simple usage examples provided.

Generate minimal dfa from provided regular expression and print number of
states.

```
$ re2fa --input-type=regexp -v -m '/a.{4}c/is'
```

Generate minimal dfa from provided file with regular expressions
(regular expressions are written line by line) and save result to the file.

```
$ re2fa --input-type=regexp-file -v -m regexp.file -o result.dfa
```

Merge multiple DFA into one.

```
$ re2fa --input-type=dfa-file -v -m re1.dfa re2.dfa re3.dfa -o result.dfa
```

## Supported regular expressions ##

Currently library supports
[PCRE](https://www.pcre.org/original/doc/html/pcrepattern.html)-like
regular expressions with some limitations.

* Regular expression starts and ends with slash '/'.

* After the final slash 'm', 's' and 'i' modifiers can be appended.

* Square brackets '[' and ']' can be used only for character class definition.

* Syntax '(?...)' is not supported.

* Simple metacharacters such as '\\', '^', '$', '.', '[', ']', '|', '(', ')',
  '?', '*', '+', '{' and '}' are fully supported.
