@echo off

mkdir ..\build
pushd ..\build
cl -DHANDMADE_WIN32=1 -FC -Zi ..\code\win32_handmade.cc user32.lib gdi32.lib
popd
