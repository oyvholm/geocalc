<!-- NEWS.md -->
<!-- File ID: d0d3fe52-9f97-11ef-a21d-83850402c3ce -->

Summary of user-visible changes in Geocalc
==========================================

For a complete log of changes, refer to the Git commit log in the 
repositories mentioned in `README.md`.

v0.x.x - 2025-xx-xx
-------------------

- Add the `-K`/`--karney` option to use the Karney formula in the `dist` 
  command, achieving sub-millimeter accuracy.
- Add the `-H`/`--haversine` option.
- Add the `sql` output format, compatible with the `sqlite3`(1) shell.
- Add the `bench` command to execute various benchmarks and report the 
  results.
- `bpos`: Print an error message if the bearing is outside the valid 
  0–360 degree range.
- Fix non-standard `srand48()` behavior on OpenBSD, where it ignores the 
  initial seed, causing `--seed` to not work because `drand48()` returns 
  non-predictable values.
- Improve various error messages.
- Print an error message if `-F gpx` is used together with output that 
  is not GPX compatible.
- Remove `errno` messages when invalid numbers were specified in certain 
  contexts.
- Remove `errno` message on FreeBSD when `--count` and `--seed` receive 
  an empty argument.
- Include the output of `--version` in the test output and send it to 
  stderr.
- Limit the output of "not ok" lines if the "randpos out of range" tests 
  fail.
- `make valgrind` now aborts properly if any Valgrind errors are found.
- Fix compiler warning on OpenBSD, where `time_t` is defined as `long 
  long`.
- Improve the output from the `make longlines` check. Also, fix it on 
  OpenBSD, where it failed due to missing UTF-8 support in `grep`(1). 
  Use Perl instead, as it handles UTF-8 properly.
- Add more warning options to DEVFLAGS in `Makefile`.
- Create a `make asm` target to generate assembly files.
- Add a new Makefile command `make gncov-refresh` to delete obsolete 
  `gncov` markers used for coverage testing.

v0.2.0 - 2024-12-27
-------------------

- Add the `randpos` command with `--count` and `--seed` options.
- Add the `--km` option.
- Add the `-F`/`--format` option, supporting `default` and `gpx`.
- `--selftest` and `--valgrind` now accept an optional argument to 
  control which test categories to run: `exec`, `func`, or `all`.
- Generate an error message when antipodal points are used with `bear`, 
  `course`, and `lpos` because the answer is undefined.
- Fix bug where `dist` printed "-nan" for certain antipodal points.
- Fix bearing calculations at the poles (±90° latitude) by moving the 
  point 1 cm against Equator to avoid computational instability.
- Normalize the longitude value from `bearing_position()`, fixing 
  certain values for `bpos` and `lpos`.
- Prevent printing of negative zero ("-0.000000"); values are now 
  normalized before output.
- Don't compile with many warning flags by default. They are only used 
  if `src/.devel` or `.git/.devel` exists, or the `DEVEL` variable is 
  set.
- Create `make cflags` to inspect the current compiler flags.
- Add `TESTS` variable for use with `make`. This variable is used as the 
  `--selftest`/`--valgrind` argument in all relevant `make` commands.
- Add `make tlokall`, which prints the result of `make tlok` with and 
  without `exec` and `func` tests.
- Add new source code checks: `make longlines` (>79 chars per line, was 
  in `testsrc`) and `make dupdesc` (duplicated test descriptions). Both 
  are now part of `make testsrc`, and `testsrc` is now part of 
  `testall`.
- `--valgrind`: Verify that Valgrind is installed on the system.
- Clean up stderr when the tests are executed, remove "got/expected" 
  output and obsolete information, and tweak descriptions.
- Add `tlokall` to the `make` command in `.gitlab-ci.yml` to document 
  the tlok status at commit time.
- `make valgrind` runs additional checks on the test process itself.

v0.1.0 - 2024-11-10
-------------------

Initial revision.

- Includes the following commands: `bear`, `bpos`, `course`, `dist`, and 
  `lpos`.
- All tests with or without Valgrind can be executed with `--selftest` 
  or `--valgrind`.

v0.0.0 - 2024-10-06
-------------------

Birth of the universe.

<!--
vim: set ts=2 sw=2 sts=2 tw=72 et fo=tcqw fenc=utf8 :
vim: set com=b\:#,fb\:-,fb\:*,n\:> ft=markdown :
-->
