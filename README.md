# Verse PLY Uploader

This program is command line tool that can be used for uploading PLY models
to Verse server.

## Requirements

* GCC http://gcc.gnu.org/ or Clang http://clang.llvm.org/
* CMake http://www.cmake.org/
* libRPLY https://github.com/jirihnidek/librply
* Verse https://github.com/verse/verse 
* OpenGL http://www.opengl.org/  (optional)
* GLUT http://www.opengl.org/resources/libraries/glut/ (optional)

## Build

This program can be build using CMake.

    $ mkdir build
    $ cd ./build
    $ cmake ../
    $ make

## Usage

You can start this command line

    $ verse_ply_uploader -f bunny.ply localhost

