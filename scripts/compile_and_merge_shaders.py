import os
import subprocess


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

DXC_PATH = os.path.join(SCRIPT_DIR, r"../.workspace/PrebuiltLibs/dxc_2024_07_31_clang_cl/bin/x64/dxc.exe")
SHADER_DIR = os.path.join(SCRIPT_DIR, r"../phoenix/src/phx/renderer/shaders")
OUTPUT_DIR = os.path.join(SCRIPT_DIR, r"../.workspace/generated_shaders")
MERGED_HEADER = os.path.join(SCRIPT_DIR, r"../Phoenix/renderer/shaders/PrecompiledShaders.h")
SHADER_MODEL = "6_6"  # Default Shader Model


def determine_entry_point(filename):
    """Determine shader entry point and type based on filename."""
    if "PS" in filename:
        return "PSMain", f"ps_{SHADER_MODEL}"
    elif "VS" in filename:
        return "VSMain", f"vs_{SHADER_MODEL}"
    return None, None


def compile_shader(shader_path, output_path, entry_point, target):
    """Compile the shader using DXC."""
    try:
        subprocess.run([
            DXC_PATH,
            "-T", target,                  # Shader target (e.g., vs_6_0)
            "-E", entry_point,             # Entry point function (e.g., PSMain or VSMain)
            "-Fo", output_path,            # Output object file
            "-Fd", output_path + ".pdb",   # Output debug info
            "-Fh", output_path + ".h",     # Output header file
            shader_path                    # Input shader file
        ], check=True)
        print(f"Compiled {shader_path} -> {output_path}.h")
    except subprocess.CalledProcessError as e:
        print(f"Failed to compile {shader_path}: {e}")


def merge_headers(output_dir, merged_header_path):
    """Merge all generated header files into a single header."""
    with open(merged_header_path, "w") as merged_file:
        merged_file.write("// Auto-generated header file. Do not edit.\n\n")
        for root, _, files in os.walk(output_dir):
            for file in files:
                if file.endswith(".h"):
                    header_path = os.path.join(root, file)
                    with open(header_path, "r") as header_file:
                        merged_file.write(header_file.read())
                        merged_file.write("\n\n")
        print(f"Merged headers into {merged_header_path}")

def main():
    """Main function to compile shaders and merge headers."""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Iterate through all HLSL files in the shader directory
    for root, _, files in os.walk(SHADER_DIR):
        for file in files:
            if file.endswith(".hlsl"):
                shader_path = os.path.join(root, file)
                filename = os.path.splitext(file)[0]
                
                # Determine entry point and target
                entry_point, target = determine_entry_point(filename)
                if not entry_point or not target:
                    print(f"Skipped {file}: Unable to determine entry point or target.")
                    continue

                # Output path for the compiled shader
                output_path = os.path.join(OUTPUT_DIR, filename)
                compile_shader(shader_path, output_path, entry_point, target)

    # Merge all generated headers
    merge_headers(OUTPUT_DIR, MERGED_HEADER)
    sys.exit(0)

if __name__ == "__main__":
    main()