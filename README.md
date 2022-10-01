# ccd2cue

## Description of this repository

This repository contains a mirror of the original
**GNU ccd2cue CCD sheet to CUE sheet converter** written by
Bruno Félix Rezende Ribeiro. The project's original source can be found at:

- https://www.gnu.org/software/ccd2cue/

This version has been modified to replace the original build script
based on **GNU Autotools** by a **CMake** build. This makes it easier to build
it on multiple plaftforms, specially those where  **GNU Autotools** is not
available.

## Limitations

For now, the script builds the code but is not creating the gettext resources
yet.
