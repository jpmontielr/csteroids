@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DCSTEROIDS_INTERNAL=1 -DCSTEROIDS_SLOW=1 -DCSTEROIDS_WIN32=1 -FC -Z7 ..\code\win32_csteroids.cpp -Fmwin32_csteroids.map /link -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

popd