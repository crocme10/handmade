@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\code\windows\main-03.cc user32.lib Gdi32.lib
popd
