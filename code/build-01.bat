@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\handmade\code\main.cc user32.lib
popd
