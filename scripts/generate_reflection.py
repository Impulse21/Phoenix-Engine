import re
import os


property_macro = re.compile(r'PROPERTY\((.*?)\)')
struct_decl = re.compile(r'struct\s+(\w+)')
member_decl = re.compile(r'([a-zA-Z0-9_:<>]+)\s+([a-zA-Z0-9_]+)\s*(=.+)?;')

def parse_property_args(arg_str):
    """Parses the PROPERTY(...) macro arguments into a dictionary."""
    args = {}
    for part in arg_str.split(','):
        if '=' in part:
            key, value = part.split('=', 1)
            args[key.strip()] = value.strip().strip('"')
    return args

def parse_def_file(filepath):
    structs = {}
    current_struct = None
    pending_property = None
    brace_depth = 0

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('//'):
                continue

            # Check for struct
            match = struct_decl.match(line)
            if match:
                current_struct = match.group(1)
                structs[current_struct] = {'properties': []}
                brace_depth = 0
                continue

            if current_struct:
                brace_depth += line.count('{')
                brace_depth -= line.count('}')
                if brace_depth <= 0:
                    current_struct = None
                    continue

                prop_match = property_macro.match(line)
                if prop_match:
                    pending_property = parse_property_args(prop_match.group(1))
                    continue

                member_match = member_decl.match(line)
                if member_match:
                    type_str, var_name, _ = member_match.groups()
                    type_str = type_str.strip()

                    if pending_property is not None:
                        property_info = {
                            'name': pending_property.get('name', var_name),
                            'tooltip': pending_property.get('tooltip', ''),
                            'extras': {k: v for k, v in pending_property.items() if k not in ('name', 'tooltip')},
                            'type': type_str,
                            'variable': var_name,
                        }
                        structs[current_struct]['properties'].append(property_info)
                        pending_property = None

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

def generate_metadata_cpp_old(structs, includes, output_filepath):
    with open(output_filepath, 'w') as out:
        for include in includes:
            out.write(f'#include \"{include}\"\n')
            
        out.write("\n\n")
        out.write('using namespace phx;\n')
        out.write('using namespace phx::rft;\n\n')

        for struct_name, struct_data in structs.items():

            out.write(f'template<>\n')
            out.write(f'struct TypeInfo<{struct_name}>\n')
            out.write('{\n')
            out.write('\tstatic constexpr FieldInfo Fields[] = {\n')

            for prop in struct_data['properties']:
                extras = ', '.join(f'{{"{k}", "{v}"}}' for k, v in prop['extras'].items())
                if not extras:
                    extras = '{}'

                out.write(f'\t\t{{ "{prop["name"]}", "{prop["type"]}", "{prop["type"]}"_hash, "{prop["tooltip"]}", offsetof({struct_name}, {prop["variable"]}), std::initializer_list<ExtraInfo>{extras} }},\n')

            out.write('\t};\n\n')
            out.write('\tstatic constexpr phx::Span<const FieldInfo> GetFields() { return Fields; }\n')
            out.write(f'\tstatic constexpr const char* GetTypeName() {{ return "{struct_name}"; }}\n')
            out.write(f'\tstatic constexpr StringHash GetTypeNameHash() {{ return "{struct_name}"_hash; }}\n')
            out.write('};\n\n')

def generate_metadata_cpp(structs, includes, output_filepath):
    with open(output_filepath, 'w') as out:
        for include in includes:
            out.write(f'#include \"{include}\"\n')
            
        out.write("\n\n")
        out.write('using namespace phx;\n')
        out.write('using namespace phx::rft;\n\n')

        for struct_name, struct_data in structs.items():
            if len(struct_data['properties']) == 0:
                continue
            out.write(f'FieldInfo {struct_name}_Fields[] = {{\n')
            for prop in struct_data['properties']:
                extras = ', '.join(f'{{"{k}", "{v}"}}' for k, v in prop['extras'].items())
                if not extras:
                    extras = '{}'

                out.write(f'\t{{ "{prop["name"]}", "{prop["type"]}"_hash, "{prop["tooltip"]}", phx_offsetof(&{struct_name}::{prop["variable"]}), nullptr, std::initializer_list<ExtraInfo>{extras} }},\n')

            out.write('};\n\n')

            out.write(f'TypeInfo {struct_name}_TypeInfo = {{\n')
            out.write(f'\t"{struct_name}", {struct_name}_Fields \n')
            out.write('};\n\n')
            out.write(f'template<> const TypeInfo& Refelction<{struct_name}>::GetTypeInfo() {{ return {struct_name}_TypeInfo; }}\n')
            out.write(f'template<> constexpr StringHash Refelction<{struct_name}>::GetTypeId() {{ return "{struct_name}"_hash; }}\n\n')

        out.write('const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {\n')
        for struct_name, struct_data in structs.items():
            if len(struct_data['properties']) == 0:
                continue
            out.write(f'\t{{ "{struct_name}", &Refelction<{struct_name}>::GetTypeInfo()}},\n')
        
        out.write('};\n\n')

def main():
    def_file_path = r"C:\Users\dipao\source\repos\Phoenix-Engine\PhxLibs\src\PhxData\WorldComponents.def.h"  # Replace with the actual path to your .def file
    output_cpp_path = r"C:\Users\dipao\source\repos\Phoenix-Engine\PhxLibs\src\PhxData\MetadataTable.generated.cpp"
    includes = ["PhxData_pch.h", "Reflection.h", "WorldComponents.def.h", "PhxCore/Base.h"]

    if not os.path.exists(def_file_path):
        print(f"Error: file not found: {def_file_path}")
        return

    parsed_structs = parse_def_file(def_file_path)
    if parsed_structs:
        #generate_metadata_header(output_h_path) # Generate the header
        generate_metadata_cpp(parsed_structs, includes, output_cpp_path)
        #print(f"Reflection header generated successfully in '{output_h_path}'")
        print("Generated:", output_cpp_path)

    else:
        print("No structs with properties found in the .def file.")

if __name__ == "__main__":
    main()