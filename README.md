# Geocalc
<!-- File ID: 93e17540-87f2-11ef-9eac-83850402c3ce -->

A command-line utility for geographic calculations using decimal 
degrees.

## Overview

**Geocalc** is a program that calculates various geographic data. It 
receives parameters and values via the command line, and prints accurate 
and strict values to standard output. This makes it suitable for use in 
scripts and quick manual calculations.

### Features

- Distance calculations between coordinates
- Bearing calculations
- Plot shortest route between points
- Generate random positions on Earth with optional distance restraints
- Recreate random sequences with initial seed value
- Calculate antipodal positions
- Output in various formats
- Minimal dependencies, no extra C libraries needed
- Platform independent
- Built-in test suite for all functionality

### Status

Contains the following commands:

- **`anti`**\
  Prints the antipodal coordinate of a position, i.e. the point on the 
  exact opposite side of the planet.
- **`bear`**\
  Prints initial compass bearing (0-360) between two points.
- **`bench`**\
  Executes various benchmarks and reports the results.
- **`bpos`**\
  Calculates the new geographic position after moving a certain amount 
  of meters or kilometers from the start position in a specific 
  direction. Negative values for the length are allowed, to make it 
  possible to calculate positions in the opposite direction of the 
  bearing.
- **`course`**\
  Generates a list of intermediate points on a direct line between two 
  locations.
- **`dist`**\
  Calculates the distance between two geographic coordinates, using the 
  Haversine or Karney formula. The result (in meters or kilometers) is 
  printed to stdout.
- **`lpos`**\
  Prints the position of a point on a straight line between the 
  positions, where `fracdist` is a fraction that specifies how far along 
  the line the point is.
- **`randpos`**\
  Generates random geographic coordinates, either uniformly distributed 
  across the globe or within or outside a specified distance range from 
  a center point. The command avoids polar bias by using a spherical 
  distribution (arcsine for latitude, uniform for longitude).

### Examples

- `geocalc bear 60.393,5.324 51.53217,-0.17786`\
  Find the bearing towards Abbey Road Studios when standing in the 
  middle of Bergen, Norway.
- `geocalc bpos 40.80542,-73.96546 188.7 4817.84`\
  Determine the new position when traveling 4817.84 meters from the 
  "Seinfeld Café" along a bearing of 188.7 degrees, heading slightly 
  southwest.
- `geocalc -F gpx course 52.3731,4.891 35.681,139.767 1000`\
  Create 1000 intermediate points on a straight line from Amsterdam to 
  Tokyo in GPX format.
- `geocalc --km dist 90,0 -90,0`\
  Calculate the distance from the North Pole to the South Pole and use 
  kilometers in the result.
- `geocalc -F gpx lpos -11.952039,49.245985 -25.606629,45.167246 0.5`\
  Find center point on Madagascar, measured from the points furthest 
  north and south. Print the result as a GPX waypoint.
- `geocalc --km --count 20 -F gpx randpos 33.33131,44.39689 12`\
  Generate 20 random locations within a radius of 12 km of Baghdad and 
  output them in GPX format.
- `geocalc -F sql --count 1000000 randpos | sqlite3 randworld.db`\
  Generate 1 million random locations around the world and store them in 
  an SQLite database.
- `(geocalc --format sql --count 50 --km randpos 55.76,37.62 20; echo 
  "SELECT * FROM randpos ORDER BY dist;") | sqlite3 -box`\
  This oneliner generates 50 random locations inside a radius of 20 km 
  around Moscow and sorts by distance.

## Development

The `master` branch is considered stable; no unstable development 
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

### Compiler flags for development

To avoid complications on various systems, the default build only uses 
`-Wall -O2`. Additional warning flags are enabled if any of the 
following conditions are met:

- The file `src/.devel` or `.git/.devel` exists
- The environment variable `DEVEL` is set to any value

These development flags can be explicitly disabled by setting the 
`NODEVEL` environment variable, regardless of the conditions above. The 
current `CFLAGS` can be checked with `make cflags`. For example:

- `make cflags`
- `make cflags DEVEL=1`
- `make cflags NODEVEL=true`

## `make` commands

### make / make all

Generate the `geocalc` executable.

### make clean

Remove all generated files except `tags`.

### make edit

Open all files in the subtree in your favourite editor defined in 
`EDITOR`.

### make asm

Generate assembly code for all `.c` files. On many Unix-like systems, 
the assembly files are stored with a `.s` extension.

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

### make gncov-refresh

Remove `gncov` markers from lines that no longer need to be excluded 
from coverage testing. This might be necessary if code changes or 
compiler optimizations make previously untestable lines testable. For 
example, if a function is inlined or optimized differently, lines that 
were once skipped might now be covered by tests.

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

`make html` creates HTML versions of all documentation in the current 
directory tree, and `make pdf` creates PDF versions. When executed in 
the `src/` directory, an HTML or PDF version of the man page is created, 
stored as `geocalc.html` or `geocalc.pdf`.

All `*.md` files can be converted to HTML or PDF by replacing the `.md` 
extension with `.html` or `.pdf`. For example, use `make README.html` to 
generate an HTML file from the `.md` file, or `make README.pdf` to 
create a PDF version. If the `.md` file is stored in Git, an extra 
footer with the text "Generated from *filename* revision *git-id* 
(*date*)" is added.

Uses `cmark`, available from <https://commonmark.org/>.

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

## About this document

This file is written in [Commonmark](https://commonmark.org) and all 
`make` commands use `cmark`(1) to generate HTML and reformat text.

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", 
"SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this 
document are to be interpreted as described in RFC 2119.

-----

<!--
vim: set ts=2 sw=2 sts=2 tw=72 et fo=tcqw fenc=utf8 :
vim: set com=b\:#,fb\:-,fb\:*,n\:> ft=markdown :
-->
