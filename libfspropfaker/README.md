fspropfaker                                                  {#mainpage}
===========

This library mounts a fuse based file system that allows one to change
single properties of the underlying file system.

Can be used in unit tests to test other programs.

Currently only supports faking the underlying disk size and free disk space size.

Most of the code is derived from

  - Joseph J. Pfeiffer's project *Big Brother File System*
  - James A. Chappell's project *FuseApp*


Usage
-----
Make sure to link to library -pthreads and set compile definition "_FILE_OFFSET_BITS=64".



Warning
-------
The API of this library isn't stable yet.
