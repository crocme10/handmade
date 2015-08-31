@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi -FAsc ..\code\windows\main-05.cc user32.lib Gdi32.lib
popd
