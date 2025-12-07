# build_assets.py

import os
import sys
import json
import subprocess
import datetime

# --- Globals ---

OUTPUT_DIRECTORY = ""
BUILD_PATH = ""

# This will be populated by querying the compilers at startup.
COMPILER_VERSIONS = {}

# A lookup map from an asset's OUTPUT path to its build rule.
ASSET_BUILD_RULES = {}

# A lookup map from an asset's INPUT path to its OUTPUT path for resolving dependencies.
INPUT_TO_OUTPUT_MAP = {}

# A set to prevent processing the same asset multiple times in a single run.
PROCESSED_ASSETS = set()

# TODO: change to have a more offical directory were we publish the asset compilers.
IMPLICIT_COMPILERS = {
    ".gltf": r".workspace/vs2022/bin/AssetCompilers/PhxModelCompiler/PhxModelCompiler.exe"
}

def resolve_paths(build_config):
    for rule in build_config['assets']:
        # os.path.join() correctly combines the paths.
        rule['input'] = os.path.join(BUILD_PATH, rule['input']).replace("\\", "/")
        rule['output'] = os.path.join(OUTPUT_DIRECTORY, rule['output']).replace("\\", "/")
        

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
            result = subprocess.run([compiler, '-version'], capture_output=True, text=True, check=True)
            version = result.stdout.strip()

            print(f"\tCompiler {compiler} version: {version}\n")
            COMPILER_VERSIONS[compiler] = version

        except (subprocess.CalledProcessError, FileNotFoundError):
            print(f"Error: Could not get version for compiler '{compiler}'. Ensure compilers are built prior to executing. Aborting.")
            sys.exit(1)
    print("-" * 30)

def get_or_create_rule_for_source(source_path):
    """Finds a build rule for a source file or creates one dynamically."""
    if source_path in INPUT_TO_OUTPUT_MAP:
        output_path = INPUT_TO_OUTPUT_MAP[source_path]
        return ASSET_BUILD_RULES[output_path]

    _, ext = os.path.splitext(source_path)
    if ext in IMPLICIT_COMPILERS:
        compiler = IMPLICIT_COMPILERS[ext]
        filename = os.path.basename(source_path)
        name_only, _ = os.path.splitext(filename)
        global OUTPUT_DIRECTORY
        output_path = f"{OUTPUT_DIRECTORY}Textures/{name_only}.phxtex"
        
        print(f"  -> Dynamic rule created for '{filename}'")
        
        new_rule = {"compiler": compiler, "input": source_path, "output": output_path}
        ASSET_BUILD_RULES[output_path] = new_rule
        INPUT_TO_OUTPUT_MAP[source_path] = output_path
        return new_rule
        
    return None

def process_asset(output_path):
    """
    Recursively processes an asset, checking all dependencies and build conditions.
    Returns True if the asset was rebuilt, False otherwise.
    """
    if output_path in PROCESSED_ASSETS:
        return False

    print(f"Checking: {os.path.basename(output_path)}")

    rule = ASSET_BUILD_RULES.get(output_path)
    if not rule:
        print(f"Warning: No build rule found for '{output_path}'. Skipping.")
        PROCESSED_ASSETS.add(output_path)
        return False

    input_path = rule['input']

    if 'compiler' in rule:
        compiler = rule['compiler']
        print(f"Using explicit compiler = {compiler}")

    else:
        _, input_ext = os.path.splitext(input_path)
        compiler = IMPLICIT_COMPILERS.get(input_ext)
        print(f"Using implicit compiler = {compiler}")

    if compiler == None:
        print(f"No Compiler found for {output_path}")
        return False

    manifest_path = output_path + ".manifest"
    needs_rebuild = False
    rebuild_reason = ""

    hard_deps_rebuilt = False
    if os.path.exists(manifest_path):
        try:
            with open(manifest_path, 'r') as f:
                manifest_data = json.load(f)

            # Process HARD dependencies to see if they force a rebuild of this asset
            # I am not sure about this. I think this will be defined else where.
            hard_deps = manifest_data.get("hard_dependencies", [])
            for src_dep in hard_deps:
                dep_rule = get_or_create_rule_for_source(src_dep)
                if dep_rule and process_asset(dep_rule['output']):
                    hard_deps_rebuilt = True

            # ALWAYS process ASSOCIATED assets to ensure they are up-to-date
            associated_assets = manifest_data.get("associated_assets", [])
            for src_dep in associated_assets:
                dep_rule = get_or_create_rule_for_source(src_dep)
                if dep_rule:
                    process_asset(dep_rule['output'])

        except (json.JSONDecodeError, FileNotFoundError):
            needs_rebuild = True
            rebuild_reason = "Manifest is corrupt or missing."

    if not needs_rebuild:
        if not os.path.exists(output_path) or not os.path.exists(manifest_path):
            needs_rebuild = True
            rebuild_reason = "Output asset or manifest is missing."

        elif not os.path.exists(input_path):
            print(f"Error: Input file '{input_path}' not found. Cannot build '{output_path}'.")
            PROCESSED_ASSETS.add(output_path)
            return False

        elif os.path.getmtime(input_path) > os.path.getmtime(output_path):
            needs_rebuild = True
            rebuild_reason = "Input file is newer than output."

        else:
            stored_version = manifest_data.get("compiler_version", "none")
            current_version = COMPILER_VERSIONS.get(compiler, "unknown")
            if stored_version != current_version:
                needs_rebuild = True
                rebuild_reason = f"Compiler version mismatch (manifest: {stored_version}, current: {current_version})."

            elif hard_deps_rebuilt:
                needs_rebuild = True
                rebuild_reason = "A hard dependency was rebuilt."

    if needs_rebuild:
        print(f"Building: {os.path.basename(output_path)}... (Reason: {rebuild_reason})")
        command = [compiler, "-i", input_path, "-o", output_path]
        try:
            print(f"\tCMD: {' '.join(command)}")
            # --- REAL IMPLEMENTATION ---
            subprocess.run(command, check=True, capture_output=True, text=True)
            
            print(f"\t-> Success.")

            with open(manifest_path, 'r') as f:
                new_manifest_data = json.load(f)
                
            associated_assets = manifest_data.get("associated_assets", [])
            if associated_assets:
                print(f"\t-> Processing newly discovered assoiciated assets...")
                for associated_asset in associated_assets:
                    asset_rule = get_or_create_rule_for_source(associated_asset)
                    if asset_rule:
                        process_asset(asset_rule['output'])
                    else:
                        print(f"\t\t- Warning: No rule found for '{asset_rule}'.")

        except Exception as e:
            print(f"\t-> ERROR: Compiler for '{output_path}' failed: {e}")
            PROCESSED_ASSETS.add(output_path)
            return False
    else:
        print(f"\t-> Up-to-date.")
    
    PROCESSED_ASSETS.add(output_path)
    return needs_rebuild

def ensure_trailing_slash(path_string):
    """
    Ensures a string has a trailing slash.

    Args:
        path_string: The input string (e.g., a file path or URL).

    Returns:
        The string with a trailing slash, if it didn't already have one.
    """
    if not path_string.endswith('/'):
        return path_string + '/'
    return path_string

# --- Main ---
def main():
    """The main entry point for the asset build pipeline."""
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

    all_compilers = {rule['compiler'] for rule in build_config['assets'] if 'compiler' in rule}
    all_compilers.update(IMPLICIT_COMPILERS.values())

    get_compiler_versions(list(all_compilers))

    global BUILD_PATH
    BUILD_PATH = os.path.dirname(os.path.abspath(build_file_path))

    global OUTPUT_DIRECTORY
    OUTPUT_DIRECTORY = BUILD_PATH
    ensure_trailing_slash(OUTPUT_DIRECTORY)

    print(f"Resolving paths relative to:\n\t->Build Path= '{BUILD_PATH}'\n\tOutput Path='{OUTPUT_DIRECTORY}'")
    resolve_paths(build_config)
    print("-" * 40)

    global ASSET_BUILD_RULES
    global INPUT_TO_OUTPUT_MAP
    for rule in build_config['assets']:
        ASSET_BUILD_RULES[rule['output']] = rule
        INPUT_TO_OUTPUT_MAP[rule['input']] = rule

    for rule in build_config['assets']:
        process_asset(rule['output'])

    print("\nAsset pipeline run complete.")

if __name__ == "__main__":
    main()