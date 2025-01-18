
import os
import shutil
import sys
import stat
import subprocess
from pathlib import Path

def extract_archive(archive_path, destination_path):
    exe_dir = os.path.join(os.getcwd(), 'vendor', "7z")
    seven_zip_exe = os.path.join(exe_dir, '7z.exe')

    # Construct the command
    cmd = f"{seven_zip_exe} x {archive_path} -o{destination_path} -aoa"

    print(f"Extracting {archive_path} to {destination_path} using: {seven_zip_exe}")

    try:
        # Execute the command
        result = subprocess.run(cmd, check=True, shell=True, text=True, capture_output=True)
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"An error occurred while extracting: {e}")
        print(f"Error output: {e.stderr}")
        raise  # Re-raise the exception for higher-level error handling if needed

    except FileNotFoundError:
        print(f"The 7z executable was not found at {seven_zip_exe}. Please check the path or installation.")
        raise

def generate_project_files():
    cmd = (
        f"vendor\\premake\\premake5.exe --file=premake5.lua {sys.argv[1]} {sys.argv[2]}"
        if sys.argv[1] == "vs2022"
        else f"premake5 --file=premake5.lua {sys.argv[1]} {sys.argv[2]}"
    )
    subprocess.Popen(cmd, shell=True).communicate()
    
    if sys.argv[1] == "vs2022" and not os.path.exists(".workspace\\vs2022\\PhxEngine.sln"):
        print("Error: PhxEngine.sln not generated.")
        sys.exit(1)
    elif sys.argv[1] != "vs2022" and not os.path.exists("Makefile") and not os.path.exists("editor/Makefile") and not os.path.exists("runtime/Makefile"):
        print("Error: makefiles not generated")
        sys.exit(1)

def main():
    is_ci = "ci" in sys.argv
    
    print("\n1. Extract Pre-built libraries...")
    extract_archive("vendor/PrebuiltLibs.7z", ".workspace/")
    
    print("\n2. Generate project files...\n")
    generate_project_files()
    
    if not is_ci:
        input("\nPress any key to continue...")
        
    sys.exit(0)

if __name__ == "__main__":
    main()