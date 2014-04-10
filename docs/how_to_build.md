How to build
============

It's easy: open the `metrics.sln` solution in Visual Studio 2010 or 2013,
choose the appropriate configuration (see below) and build it - that's it.

Binaries will be built in `build\` directory, in a subdirectory depending on
configuration. E.g. if you build `Debug` configuration, you will find files in
`build\debug` directory.

The provided solution file contains two projects:

* `metrics` - a demo console app which contains metric++ library files
* `test` - unit tests for metric++

There are configurations both for Visual Studio 2010 and 2013:

* use `Release` and `Debug` configurations to build on VS 2013
* use `Release_VS2010` and `Debug_VS2010` to build on VS 2010

Metric++ is provided as a set of source code files. It is up to you to decide
how to add it to your project: as source files, as lib/dll or something else.

### Documentation

Metric++ is thoroughly documented.

To generate the full doxygen documentation for the library, execute `build_docs.bat`
script in the project's root folder. The documentation will be generated in
directory `build\docs\html`, just open `index.html` file.
