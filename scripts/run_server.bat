@echo off
if exist build\bin\r-type_server.exe (
    start "r-type_server" powershell -NoExit -Command .\build\bin\r-type_server.exe
) else (
    echo Executable not found. Build first with CMake.
    pause
)
