import os
import sys
import json
import subprocess
import datetime

COMPILER_VERSIONS = {}

# A lookup map for finding the build rule for a given output file.
# This is populated from the main asset build file.
ASSET_BUILD_RULES = {}

IMPLICIT_COMPILERS = {
    ".gltf": "C:/PhoenixEngine/bin/PhxModelCompiler.exe",
    ".png": "C:/PhoenixEngine/bin/PhxTextureCompiler.exe",
    ".jpg": "C:/PhoenixEngine/bin/PhxTextureCompiler.exe"
}

# --- Pretty Banner ---
def print_banner():
    banner = r"""
██████╗ ██╗  ██╗██╗  ██╗
██╔══██╗██║  ██║╚██╗██╔╝
██████╔╝███████║ ╚███╔╝ 
██╔═══╝ ██╔══██║ ██╔██╗ 
██║     ██║  ██║██╔╝ ██╗
╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝

"""
    width = 70

    print("=" * width)
    
    # Split the banner into individual lines and center each one
    for line in banner.splitlines():
        print(line.center(width))
        
    print("PHX Engine Asset Pipeline".center(width))
    print("=" * width)
    print()

def get_compiler_versions(compilers):
    """
    Queries all known compilers for their versions.
    In a real implementation, this would use subprocess to call the executables.
    """
    print("Querying compiler versions...")
    global COMPILER_VERSIONS
    for compiler in compilers:
        try:
            result = subprocess.run([compiler, '--version'], capture_output=True, text=True, check=True)
            version = result.stdout.strip()
            COMPILER_VERSIONS[compiler] = version

        except (subprocess.CalledProcessError, FileNotFoundError):
            print(f"Error: Could not get version for compiler '{compiler}'. Aborting.")
            sys.exit(1)
    print("-" * 30)

# --- Main ---
def main():
    print_banner()

    if len(sys.argv) < 2:
        print("Usage: python build_assets.py <path_to_build_file.json>")
        sys.exit(1)


    build_file_path = sys.argv[1]

    if not os.path.exists(build_file_path):
        print(f"Error: Build file not found at '{build_file_path}'")
        sys.exit(1)

    with open(build_file_path, 'r') as f:
        build_config = json.load(f)

    all_compilers = {rule['compiler'] for rule in build_config['assets']}
    all_compilers.update(IMPLICIT_COMPILERS.values())
    get_compiler_versions(list(all_compilers))
    
    print("\nAsset pipeline run complete.")

if __name__ == "__main__":
    main()