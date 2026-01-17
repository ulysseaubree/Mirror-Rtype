#!/usr/bin/env python3
import os
import subprocess
import sys
import platform
import shutil

def run_command(command, check=True, cwd=None):
    """Exécute une commande de manière sécurisée."""
    cmd_str = ' '.join(command) if isinstance(command, list) else command
    print(f"\033[94m>> Executing: {cmd_str}\033[0m")
    try:
        subprocess.run(command, check=check, shell=(platform.system() == "Windows" and isinstance(command, str)), cwd=cwd)
    except subprocess.CalledProcessError as e:
        print(f"\033[91m[ERROR] Command failed: {e}\033[0m")
        sys.exit(1)

def main():
    print("\033[92m=== R-Type Engine Bootstrap ===\033[0m")
    root_dir = os.getcwd()
    vcpkg_dir = os.path.join(root_dir, "vcpkg")
    build_dir = os.path.join(root_dir, "build")

    # 1. Gestion du Submodule Vcpkg
    if not os.path.exists(os.path.join(vcpkg_dir, "README.md")):
        print("initializing vcpkg submodule...")
        run_command(["git", "submodule", "update", "--init", "--recursive"])

    # 2. Bootstrap Vcpkg Executable
    vcpkg_exe = "vcpkg.exe" if platform.system() == "Windows" else "vcpkg"
    if not os.path.exists(os.path.join(vcpkg_dir, vcpkg_exe)):
        script = "bootstrap-vcpkg.bat" if platform.system() == "Windows" else "./bootstrap-vcpkg.sh"
        print(f"Bootstrapping vcpkg with {script}...")
        run_command([os.path.join(vcpkg_dir, script)] if platform.system() != "Windows" else os.path.join(vcpkg_dir, script), cwd=vcpkg_dir)

    # 3. Nettoyage du cache (Crucial après changement de structure CMake)
    if os.path.exists(os.path.join(build_dir, "CMakeCache.txt")):
        print("Refactoring detected: Cleaning CMake cache...")
        try:
            shutil.rmtree(build_dir)
        except Exception as e:
            print(f"Warning: Could not remove build dir: {e}")

    os.makedirs(build_dir, exist_ok=True)

    # 4. Configuration CMake
    print("Configuring CMake...")
    
    # Détection Ninja
    has_ninja = shutil.which("ninja") is not None
    preset_name = "default" # Défini dans CMakePresets.json
    
    cmd_cmake = ["cmake", "--preset", preset_name]
    
    # Fallback si Ninja n'est pas là et que le preset le force (optionnel, dépend de ton json)
    if not has_ninja and platform.system() != "Windows":
        print("\033[93mNinja not found, attempting standard configuration...\033[0m")
        # Override manuel si le preset échoue
        cmd_cmake = ["cmake", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=Debug", f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_dir}/scripts/buildsystems/vcpkg.cmake"]

    run_command(cmd_cmake)

    # 5. Compilation
    print("Building targets...")
    run_command(["cmake", "--build", "build"])

    print("\n\033[92m=== SUCCESS ===\033[0m")
    print("Core library compiled in 'build/lib'")
    print("Executables will appear in 'build/bin' once 'client/' or 'server/' sources are created.")

if __name__ == "__main__":
    main()
