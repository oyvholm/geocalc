# README for geocalc.git

## About this document

This file is written in [Commonmark](https://commonmark.org) and all 
`make` commands use `cmark`(1) to generate HTML and reformat text.

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", 
"SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this 
document are to be interpreted as described in RFC 2119.

## Status

Contains the following commands:

  - **`bear`**
    - Print initial compass bearing (0-360) between two points.
  - **`bpos`**
    - Calculates the new geographic position after moving a certain 
      amount of meters from the start position in a specific direction. 
      Negative values for the length are allowed, to make it possible to 
      calculate positions in the opposite direction of the bearing.
  - **`dist`**
    - Calculate the distance between two geographic coordinates, using 
      the Haversine formula. The result (in meters) is printed to 
      stdout.

## Development

The `master` branch is considered stable, no unstable development 
happens there. Every new functionality or bug fix is created on topic 
branches which may be rebased now and then. All tests on `master` 
(executed with "make test") MUST succeed. If any test fails, it's 
considered a bug. Please report any failing tests in the issue tracker.

To ensure compatibility between versions, the program follows the 
Semantic Versioning Specification described at <http://semver.org>. 
Using the version number `X.Y.Z` as an example:

  - `X` is the *major version*.
    This number is only incremented when backwards-incompatible changes 
    are introduced.
  - `Y` is the *minor version*.
    Increased when new backwards-compatible features are added.
  - `Z` is the *patch level*.
    Increased when new backwards-compatible bugfixes are added.

## `make` commands

### make / make all

Generate the `geocalc` executable.

### make clean

Remove all generated files except `tags`.

### make edit

Open all files in the subtree in your favourite editor defined in 
`EDITOR`.

### make gcov

Generate test coverage with `gcov`(1). Should be as close to 100% as 
possible.

### make gcov-cmt / make gcov-cmt-clean

Add or remove `gcov` markers in the source code in lines that are not 
tested. Lines that are hard to test, for example full disk, full memory, 
long paths and so on, can be marked with the string `/* gncov */` to 
avoid marking them. To mark lines even when marked with gncov, set the 
GNCOV environment variable to a non-empty value. For example:

    make gcov-cmt GNCOV=1

### make gdb

Start gdb with main() as the default breakpoint, this is defined in 
`src/gdbrc`. Any additional gdb options can be added in `src/gdbopts`. 
An example would be "-tty /dev/\[...\]" to send the program output to 
another window.

### make install

`make install` installs `geocalc` to the location defined by `PREFIX` in 
`src/Makefile`. Default location is `/usr/local`, but it can be 
installed somewhere else by specifying `PREFIX`. For example:

    make install PREFIX=~/local

### make tags

Generate `tags` file, used by Vim and other editors.

### make test

Run all tests. This command MUST NOT fail on purpose on `master`.

### make uninstall

Delete the installed version from `PREFIX`.

### make valgrind

Run all tests with Valgrind to find memory leaks and other problems. 
Should also not fail on master.

### Create HTML or PDF

All `*.md` files can be converted to HTML or PDF by replacing the `.md` 
extension with `.html` or `.pdf`. For example, use `make README.html` to 
generate an HTML file from the `.md` file, or `make README.pdf` to 
create a PDF version. If the `.md` file is stored in Git, an extra 
footer with the text "Generated from *filename* revision *git-id* 
(*date*)" is added.

Uses `cmark`, available from <https://commonmark.org/>.

`make html` creates HTML versions of all documentation in the current 
directory tree, and `make pdf` creates PDF versions. When executed in 
the `src/` directory, an HTML or PDF version of the man page is created, 
stored as `geocalc.html` or `geocalc.pdf`.

## Download

The main Git repository is stored at GitLab:

  - URL: <https://gitlab.com/oyvholm/geocalc>
  - SSH clone: git@gitlab.com:oyvholm/geocalc.git
  - https clone: <https://gitlab.com/oyvholm/geocalc.git>

## License

This program is free software; you can redistribute it and/or modify it 
under the terms of the GNU General Public License as published by the 
Free Software Foundation; either version 2 of the License, or (at your 
option) any later version.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program. If not, see <http://www.gnu.org/licenses/>.

## Author

Øyvind A. Holm \<<sunny@sunbase.org>\>

-----

File ID: 93e17540-87f2-11ef-9eac-83850402c3ce

<!--
vim: set ts=2 sw=2 sts=2 tw=72 et fo=tcqw fenc=utf8 :
vim: set com=b\:#,fb\:-,fb\:*,n\:> ft=markdown :
-->
