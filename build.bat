@echo off
setlocal

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

set "CMAKE_EXE=cmake"
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "D:\Files\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=D:\Files\CMake\bin\cmake.exe"
    ) else (
        echo CMake was not found. Install CMake and add it to PATH first.
        exit /b 1
    )
)

"%CMAKE_EXE%" -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b %errorlevel%

"%CMAKE_EXE%" --build "%PROJECT_DIR%\build"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Built: %PROJECT_DIR%\build\C++Renderer.exe
