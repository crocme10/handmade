@echo off

mkdir ..\build
pushd ..\build
cl -Zi ..\code\main.cc user32.lib
popd
