import os
import subprocess
import argparse

def run_clang_tidy(file_path, build_dir):
    print(f"Running clang-tidy on: {file_path}")
    try:
        subprocess.run([
            "clang-tidy",
            "-p", build_dir,
            "-fix",
            "-fix-errors",
            file_path
        ], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error processing {file_path}:\n{e}")

def process_directory(source_dir, build_dir):
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if file.endswith((".cpp", ".c", ".h", ".hpp", ".cc", ".cxx")):
                full_path = os.path.join(root, file)
                run_clang_tidy(full_path, build_dir)

def main():
    parser = argparse.ArgumentParser(description="Run clang-tidy with -fix on all source files.")
    parser.add_argument("--source", required=True, help="Path to the source directory.")
    parser.add_argument("--build", required=True, help="Path to the build directory containing compile_commands.json.")

    args = parser.parse_args()

    if not os.path.isfile(os.path.join(args.build, "compile_commands.json")):
        print("Error: compile_commands.json not found in build directory.")
        return

    process_directory(args.source, args.build)

if __name__ == "__main__":
    main()
