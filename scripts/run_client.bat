@echo off
if exist build\bin\r-type_client.exe (
    start "r-type_client" powershell -NoExit -Command .\build\bin\r-type_client.exe
) else (
    echo Executable not found. Build first with CMake.
    pause
)
