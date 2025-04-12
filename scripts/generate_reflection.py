import re
import os
import argparse
import glob

property_macro = re.compile(r'PROPERTY\((.*?)\)')
struct_decl = re.compile(r'struct\s+(\w+)')
member_decl = re.compile(r'([a-zA-Z0-9_:<>]+(?:<[^>]+>)?)\s+([a-zA-Z0-9_]+)\s*(=.+)?;')


def glob_def_h_files(directory):
    pattern = os.path.join(directory, "*.def.h")
    matching_files = glob.glob(pattern)
    return [os.path.abspath(path) for path in matching_files] 

def parse_type(type_str: str):
    type_str = type_str.strip()

    # Check for raw pointer
    is_pointer = '*' in type_str
    type_str = type_str.replace('*', '').strip()

    # Check for smart pointer-like templates
    smart_pointer_types = ['RefCountPtr', 'std::shared_ptr', 'UniquePtr']
    for smart_type in smart_pointer_types:
        if type_str.startswith(smart_type + "<"):
            is_pointer = True
            # Extract inner type, e.g., RefCountPtr<Foo> → Foo
            inner_type = re.findall(r'<(.+?)>', type_str)
            if inner_type:
                type_str = inner_type[0].strip()
            break

    return type_str, is_pointer

def parse_property_args(arg_str: str):
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
                    full_type, var_name, _ = member_match.groups()
                    base_type, is_pointer = parse_type(full_type)

                    if pending_property is not None:
                        property_info = {
                            'name': pending_property.get('name', var_name),
                            'tooltip': pending_property.get('tooltip', ''),
                            'extras': {k: v for k, v in pending_property.items() if k not in ('name', 'tooltip')},
                            'type': full_type,
                            'variable': var_name,
                            "is_pointer":is_pointer
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

def generate_metadata_cpp(structs, includes, output_filepath):
    with open(output_filepath, 'w') as out:
        for include in includes:
            out.write(f'#include \"{include}\"\n')
            
        out.write("\n\n")
        out.write('using namespace phx;\n')
        out.write('using namespace phx::data;\n\n')

        for struct_name, struct_data in structs.items():
            if len(struct_data['properties']) == 0:
                continue
            out.write(f'FieldInfo {struct_name}_Fields[] = {{\n')
            for prop in struct_data['properties']:
                extras = ', '.join(f'{{"{k}", "{v}"}}' for k, v in prop['extras'].items())
                if not extras:
                    extras = '{}'
            
                is_pointer_cpp = 'true' if prop["is_pointer"] else 'false'
                out.write(f'\t{{ "{prop["name"]}", "{prop["type"]}"_hash, "{prop["tooltip"]}", phx_offsetof(&{struct_name}::{prop["variable"]}), std::initializer_list<ExtraInfo>{{{extras}}}, {is_pointer_cpp} }},\n')

            out.write('};\n\n')

            out.write(f'TypeInfo {struct_name}_TypeInfo = {{\n')
            out.write(f'\t"{struct_name}", {struct_name}_Fields \n')
            out.write('};\n\n')
            out.write(f'template<> const TypeInfo& Reflection<{struct_name}>::GetTypeInfo() {{ return {struct_name}_TypeInfo; }}\n')
            out.write(f'REGISTER_TYPE_FACTORY({struct_name})\n\n')

        out.write('const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {\n')
        for struct_name, struct_data in structs.items():
            if len(struct_data['properties']) == 0:
                continue
            out.write(f'\t{{ "{struct_name}", &Reflection<{struct_name}>::GetTypeInfo()}},\n')
        
        out.write('};\n\n')

def main():
        
    includes = ["PhxData_pch.h", "Reflection.h", "DataTypeFactory.h", "PhxCore/Base.h"]

    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("headers_dir", nargs="+")
    args = parser.parse_args()

    header_files = []
    for headers_dir in args.headers_dir:
        header_file_path = os.path.abspath(headers_dir)
        def_files = glob_def_h_files(header_file_path)
        header_files.extend(def_files)

    parsed_structs = {}
    for header_file in header_files:
        header_file_abs = os.path.abspath(header_file)
        if not os.path.exists(header_file_abs):
            print(f"Error: file not found: {header_file_abs}")
            return

        parsed_data = parse_def_file(header_file_abs)
        parsed_structs.update(parsed_data)
        
        include_entry = os.path.basename(header_file)
        includes.append(f"PhxData/{include_entry}")

    if parsed_structs:
        #generate_metadata_header(output_h_path) # Generate the header
        output_path = os.path.join(os.getcwd(), os.path.abspath(args.output))
        generate_metadata_cpp(parsed_structs, includes, output_path)
        #print(f"Reflection header generated successfully in '{output_h_path}'")
        print("Generated:", output_path)

    else:
        print("No structs with properties found in the .def file.")

if __name__ == "__main__":
    main()