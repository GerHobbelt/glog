@echo off

CALL :GLOG

EXIT /B %ERRORLEVEL%



:GLOG
set SRC_DIR=%~dp0
cd %SRC_DIR%
rd /s /q %SRC_DIR%\output
rd /s /q %SRC_DIR%\build
cmake -B output -DBUILD_SHARED_LIBS=ON -DCMAKE_GENERATOR_TOOLSET=v142,host=x64 -DCMAKE_SYSTEM_VERSION=10.0.18362.0 -G "Visual Studio 16 2019"  -DBUILD_TESTING=OFF -DHAVE_DBGHELP=FALSE -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=..\\3rd-parties\\vendors\\glog\\win\\x64\\Debug .
cmake --build ./output --config Debug --target install

rd /s /q %SRC_DIR%\output
rd /s /q %SRC_DIR%\build
cmake -B output -DBUILD_SHARED_LIBS=ON -DCMAKE_GENERATOR_TOOLSET=v142,host=x64 -DCMAKE_SYSTEM_VERSION=10.0.18362.0 -G "Visual Studio 16 2019"  -DBUILD_TESTING=OFF -DHAVE_DBGHELP=FALSE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=..\\3rd-parties\\vendors\\glog\\win\\x64\\Release .
cmake --build ./output --config Release --target install

EXIT /B 0

