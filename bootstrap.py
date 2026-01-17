#!/usr/bin/env python3
import os
import subprocess
import sys
import platform
import shutil
import stat

def run_command(command, cwd=None, shell=False):
    """Exécute une commande de manière sécurisée et affiche la sortie."""
    # Conversion en string pour l'affichage uniquement
    cmd_str = ' '.join(command) if isinstance(command, list) else command
    print(f"\033[94m>> Executing: {cmd_str}\033[0m")
    
    try:
        # Sur Windows, pour exécuter des .bat ou commandes système, shell=True est souvent requis
        # si la commande est une liste, on laisse subprocess gérer les arguments sauf cas spécifiques
        use_shell = shell or (platform.system() == "Windows" and isinstance(command, str))
        
        subprocess.run(command, check=True, cwd=cwd, shell=use_shell)
    except subprocess.CalledProcessError as e:
        print(f"\033[91m[ERROR] Command failed with return code {e.returncode}\033[0m")
        sys.exit(1)

def make_executable(path):
    """Rend un fichier exécutable (équivalent chmod +x) sur Unix."""
    if platform.system() != "Windows" and os.path.exists(path):
        st = os.stat(path)
        os.chmod(path, st.st_mode | stat.S_IEXEC)

def main():
    print("\033[92m=== R-Type Engine Bootstrap ===\033[0m")
    
    # Détection de l'OS
    is_windows = platform.system() == "Windows"
    root_dir = os.getcwd()
    vcpkg_dir = os.path.join(root_dir, "vcpkg")
    build_dir = os.path.join(root_dir, "build")

    # ---------------------------------------------------------
    # 1. Gestion du Submodule Vcpkg
    # ---------------------------------------------------------
    if not os.path.exists(os.path.join(vcpkg_dir, "README.md")):
        print("Initializing vcpkg submodule...")
        run_command(["git", "submodule", "update", "--init", "--recursive"])

    # ---------------------------------------------------------
    # 2. Bootstrap Vcpkg Executable
    # ---------------------------------------------------------
    vcpkg_exe_name = "vcpkg.exe" if is_windows else "vcpkg"
    vcpkg_exe_path = os.path.join(vcpkg_dir, vcpkg_exe_name)

    if not os.path.exists(vcpkg_exe_path):
        print("Bootstrapping vcpkg...")
        if is_windows:
            script_path = os.path.join(vcpkg_dir, "bootstrap-vcpkg.bat")
            # Windows requiert souvent shell=True pour les .bat
            run_command([script_path], cwd=vcpkg_dir, shell=True)
        else:
            script_path = os.path.join(vcpkg_dir, "bootstrap-vcpkg.sh")
            make_executable(script_path)
            run_command([script_path], cwd=vcpkg_dir)

    # ---------------------------------------------------------
    # 3. Nettoyage du cache
    # ---------------------------------------------------------
    # On nettoie si on détecte un changement majeur ou pour forcer une config propre
    if os.path.exists(os.path.join(build_dir, "CMakeCache.txt")):
        print("Cleaning previous CMake cache...")
        try:
            shutil.rmtree(build_dir)
        except Exception as e:
            print(f"Warning: Could not remove build dir: {e}")

    os.makedirs(build_dir, exist_ok=True)

    # ---------------------------------------------------------
    # 4. Configuration CMake
    # ---------------------------------------------------------
    print("Configuring CMake...")
    
    # Chemin vers le toolchain file de vcpkg
    vcpkg_toolchain = os.path.join(vcpkg_dir, "scripts/buildsystems/vcpkg.cmake")
    
    # Stratégie : On essaie une config standard solide au lieu de dépendre aveuglément des presets
    # qui peuvent échouer si Ninja n'est pas installé sur Windows.
    
    cmake_config_cmd = [
        "cmake",
        "-S", ".",
        "-B", "build",
        f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_toolchain}"
    ]

    # Ajout du Build Type (utile pour Ninja/Makefiles, ignoré par Visual Studio)
    cmake_config_cmd.append("-DCMAKE_BUILD_TYPE=Debug")

    run_command(cmake_config_cmd)

    # ---------------------------------------------------------
    # 5. Compilation
    # ---------------------------------------------------------
    print("Building targets...")
    
    # La commande --config Debug est CRUCIALE pour Visual Studio (Windows)
    # Pour Makefiles/Ninja, elle est ignorée si CMAKE_BUILD_TYPE est déjà set, donc c'est safe.
    build_cmd = ["cmake", "--build", "build", "--config", "Debug"]
    
    # Optionnel : Paralléliser le build
    build_cmd.extend(["--parallel", str(os.cpu_count())])

    run_command(build_cmd)

    print("\n\033[92m=== SUCCESS ===\033[0m")
    if is_windows:
        print("Binaries are likely in 'build/Debug/' or 'build/bin/'")
    else:
        print("Binaries are in 'build/' or 'build/bin/'")

if __name__ == "__main__":
    main()
