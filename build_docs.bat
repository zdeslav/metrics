@echo off

set DOXY_TOOL=tools\doxygen\doxygen.exe
set PROJECT_ROOT=%~dp0

echo.

echo Building source code documentation..
echo.

cd %PROJECT_ROOT%

echo deleting contents of %PROJECT_ROOT%build\docs

RD /S /Q %PROJECT_ROOT%build\docs
MD  %PROJECT_ROOT%build\docs
echo.


%DOXY_TOOL% metrics.doxy

