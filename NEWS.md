<!-- NEWS.md -->
<!-- File ID: d0d3fe52-9f97-11ef-a21d-83850402c3ce -->

Summary of changes in Geocalc
=============================

For a complete log of changes, please refer to the Git commit log in the 
repositories mentioned in `README.md`.

v0.x.x - 202x-xx-xx
-------------------

- Add the `--km` option.
- Add the `-F`/`--format` option. Supports `default` and `gpx`.
- `--selftest` and `--valgrind` now accept an optional argument to 
  control which test categories to run: `exec`, `func`, or `all`.
- Generate error message when antipodal points are used with `bear`, 
  `course` and `lpos` because the answer is undefined.
- Fix bug when `dist` printed "-nan" for certain antipodal points.
- Fixed bearing calculations near poles (±90° latitude) by moving the 
  point 1 cm against Equator to avoid computational instability.
- Add `TESTS` variable for use with `make`. This variable is used as 
  `--selftest`/`--valgrind` argument in all relevant `make` commands.
- Add `make tlokall`, prints the result of `make tlok` with and without 
  `exec` and `func` tests.
- `--valgrind`: Verify that Valgrind is installed on the system.
- Clean up stderr when the tests are executed, remove "got/expected" 
  output and obsolete info, tweak descriptions.
- Rename `string.c` to `strings.c`.
- Add `tlokall` to the `make` command in `.gitlab-ci.yml` to document 
  the tlok status at commit time.

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
