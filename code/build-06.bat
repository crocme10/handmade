@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi -FAsc ..\code\windows\main-06.cc user32.lib Gdi32.lib
popd
