# Build & Run (Windows)

Prerequisites:
- CMake >= 3.16
- A C++17-compatible compiler (MSVC or clang/gcc)
- SFML 2.5 (install system-wide or via vcpkg)

Quick build (from project root):

```powershell
mkdir build
cd build
cmake -S .. -B . -A x64
cmake --build . --config Debug
```

Run the server and client (after building):

```powershell
start powershell -NoExit -Command .\\build\\bin\\r-type_server.exe
start powershell -NoExit -Command .\\build\\bin\\r-type_client.exe
```

If you prefer convenience scripts, see the root files `run_server.bat`, `run_client.bat`, and `run_both.bat` which start each executable in a new window (assumes `build\\bin` contains the targets).

If SFML is not found, consider installing via vcpkg and passing the toolchain file to CMake:

```powershell
cmake -S .. -B . -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Troubleshooting:
- If CMake cannot find SFML, ensure SFML is installed and its libraries are discoverable or use vcpkg.
- On first run, allow the application through any firewall if networking is added later.
