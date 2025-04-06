import re
import os

def parse_def_file(filepath):
    """Parses a .def file and extracts struct and property information."""
    structs = {}
    current_struct = None

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('//'):
                continue

            struct_match = re.match(r'struct (\w+)', line)
            if struct_match:
                print("struct Match")
                current_struct = struct_match.group(1)
                structs[current_struct] = {'properties': []}
                continue

            if current_struct:
                if '}' in line:
                    current_struct = None
                    continue

                property_match = re.match(r'PROPERTY\(name="([^"]*)",\s*tooltip="([^"]*)"\)\s+(.+)\s+([a-zA-Z0-9_]+)\s*=?.*?;', line)
                if property_match:
                    name, tooltip, type_str, var_name = property_match.groups()
                    structs[current_struct]['properties'].append({
                        'name': name,
                        'tooltip': tooltip,
                        'type': type_str.strip(),
                        'variable': var_name
                    })
                    continue

                # Basic type and variable extraction (without PROPERTY macro)
                member_match = re.match(r'([a-zA-Z0-9_:]+)\s+([a-zA-Z0-9_]+)\s*=?.*?;', line)
                if member_match:
                    type_str, var_name = member_match.groups()
                    # We only want to include these if they are not part of a PROPERTY macro
                    is_property = False
                    for prop in structs.get(current_struct, {}).get('properties', []):
                        if prop['variable'] == var_name:
                            is_property = True
                            break
                    if not is_property:
                        structs[current_struct].setdefault('members', []).append({
                            'type': type_str.strip(),
                            'variable': var_name
                        })

    return structs

def generate_metadata_header(output_filepath):
    """Generates the C++ .h metadata header file."""
    with open(output_filepath, 'w') as outfile:
        outfile.write("#pragma once\n")
        outfile.write("#include <unordered_map>\n")
        outfile.write("#include <string>\n")
        outfile.write("#include <cstddef> // For std::size_t\n\n")
        outfile.write("namespace phx::rfl \n{\n\n")
        outfile.write("struct PropertyInfo \n{\n")
        outfile.write("    const char* name;\n")
        outfile.write("    const char* tooltip;\n")
        outfile.write("    const char* typeName;\n")
        outfile.write("    std::size_t offset;\n")
        outfile.write("};\n\n")
        outfile.write("struct StructInfo \n{\n")
        outfile.write("    const char* name;\n")
        outfile.write("    std::unordered_map<std::string, PropertyInfo> properties;\n")
        outfile.write("};\n\n")
        outfile.write("extern std::unordered_map<std::string, StructInfo> g_metadata;\n\n")
        outfile.write("} // namespace Reflection\n\n")

def generate_metadata_cpp(structs, output_header, output_filepath):
    """Generates the C++ .cpp metadata table."""
    with open(output_filepath, 'w') as outfile:
        outfile.write(f"#include {output_header}\n\n")
        outfile.write("using namespace phx::rfl;\n\n")

        outfile.write("std::unordered_map<std::string, StructInfo> g_metadata = \n{\n")

        for struct_name, struct_data in structs.items():
            outfile.write(f'    {{ "{struct_name}", {{ "{struct_name}", {{\n')
            for prop in struct_data.get('properties', []):
                # Calculate offset (this is a placeholder, actual offset calculation is complex)
                outfile.write(f'        {{ "{prop["name"]}", {{ "{prop["name"]}", "{prop["tooltip"]}", "{prop["type"]}", offsetof({struct_name}, {prop["variable"]}) }} }},\n')
            outfile.write("    }} }}\n")
        outfile.write("};\n\n")

def main():
    def_file_path = r"C:\Users\dipao\source\repos\Phoenix-Engine\PhxLibs\src\PhxData\WorldComponents.def"  # Replace with the actual path to your .def file
    output_cpp_path = r"C:\Users\dipao\source\repos\Phoenix-Engine\PhxLibs\src\PhxData\MetadataTable.generated.cpp"
    output_h_path = r"C:\Users\dipao\source\repos\Phoenix-Engine\PhxLibs\src\PhxData\Reflection.generated.h"
    if not os.path.exists(def_file_path):
        print(f"Error: .def file not found at '{def_file_path}'")
    else:
        parsed_structs = parse_def_file(def_file_path)
        if parsed_structs:
            generate_metadata_header(output_h_path) # Generate the header
            generate_metadata_cpp(parsed_structs, output_h_path, output_cpp_path)

            print(f"Reflection header generated successfully in '{output_h_path}'")
            print(f"Metadata generated successfully in '{output_cpp_path}'")
        else:
            print("No structs with properties found in the .def file.")

if __name__ == "__main__":
    main()