<!-- NEWS.md -->
<!-- File ID: d0d3fe52-9f97-11ef-a21d-83850402c3ce -->

Summary of changes in Geocalc
=============================

For a complete log of changes, please refer to the Git commit log in the 
repositories mentioned in `README.md`.

v0.x.x - 2025-xx-xx
-------------------

- Add the `-K`/`--karney` option to use the Karney formula in the `dist` 
  command, achieving sub-millimeter accuracy.
- Add the `-H`/`--haversine` option.
- Add `sql` output format, compatible with the `sqlite3`(1) shell.
- Add the `bench` command, executes various benchmarks and reports the 
  results.
- Create a `make asm` target to generate assembly files.
- Remove errno messages if invalid numbers were specified certain 
  places.
- Remove errno message on FreeBSD when `--count` and `--seed` got empty 
  arg.
- Send the output of `--version` to stderr in the test output.
- Limit the output of "not ok" lines if the "randpos out of range" tests 
  fail.
- Various changes and fixes, no functional changes.

v0.2.0 - 2024-12-27
-------------------

- Add the `randpos` command with the `--count` and `--seed` option.
- Add the `--km` option.
- Add the `-F`/`--format` option. Supports `default` and `gpx`.
- `--selftest` and `--valgrind` now accept an optional argument to 
  control which test categories to run: `exec`, `func`, or `all`.
- Generate error message when antipodal points are used with `bear`, 
  `course` and `lpos` because the answer is undefined.
- Fix bug where `dist` printed "-nan" for certain antipodal points.
- Fix bearing calculations at the poles (±90° latitude) by moving the 
  point 1 cm against Equator to avoid computational instability.
- Normalize the longitude value from `bearing_position()`, fixes certain 
  values for `bpos` and `lpos`.
- Don't print negative zero ("-0.000000"), normalize before output.
- Don't compile with lots of warning flags by default. Only use them if 
  `src/.devel` or `.git/.devel` exists, or the `DEVEL` variable is set.
- Create `make cflags` to inspect the current compiler flags.
- Add `TESTS` variable for use with `make`. This variable is used as 
  `--selftest`/`--valgrind` argument in all relevant `make` commands.
- Add `make tlokall`, prints the result of `make tlok` with and without 
  `exec` and `func` tests.
- Add new source code checks: `make longlines` (>79 chars per line, was 
  in `testsrc`) and `make dupdesc` (duplicated test descriptions). Both 
  are now part of `make testsrc`, and `testsrc` is now part of 
  `testall`.
- `--valgrind`: Verify that Valgrind is installed on the system.
- Clean up stderr when the tests are executed, remove "got/expected" 
  output and obsolete info, tweak descriptions.
- Add `tlokall` to the `make` command in `.gitlab-ci.yml` to document 
  the tlok status at commit time.
- `make valgrind` runs additional checks on the test process itself.

v0.1.0 - 2024-11-10
-------------------

Initial revision.

- Contains these commands: `bear`, `bpos`, `course`, `dist`, `lpos`.
- All tests with or without Valgrind can be executed with `--selftest` 
  or `--valgrind`.

v0.0.0 - 2024-10-06
-------------------

Birth of the universe.

<!--
vim: set ts=2 sw=2 sts=2 tw=72 et fo=tcqw fenc=utf8 :
vim: set com=b\:#,fb\:-,fb\:*,n\:> ft=markdown :
-->
