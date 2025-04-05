import re
import sys
from pathlib import Path


property_regex = re.compile(r'PROPERTY\s*\((.*?)\)\s*')

def parse_defines_header_T(file):
    
    structs = {}
    current = None
    for line in file.readlines():
        line = line.strip()
        
        if line.startswith("struct") and current is not None:
            print("Found Struct")
            current["name"] = re.findall(r"struct (\w+)", line)[0]
            print(current["name"])
            continue

        if line.startswith("PROPERTY" ):
            attr = re.findall(r"\[@(\w+)\((.*?)\)\]", line)[0]
            current.setdefault("pending_attrs", []).append(attr)
            continue

        m = re.match(r"(\w[\w<>:]*)\s+(\w+);", line)
        if m and current:
            ftype, fname = m.groups()
            field = {"type": ftype, "name": fname, "attrs": []}
            if "pending_attrs" in current:
                field["attrs"] = current["pending_attrs"]
                current.pop("pending_attrs")
            current["fields"].append(field)

        if line == "};" and current:
            structs[current["name"]] = current
            current = None

    return structs


def parse_defines_header(file):
    
    structs = {}
    current = None
    for line in file.readlines():
        line = line.strip()
        
        if line.startswith("struct") and current is None:
            print("Found Struct")
            print(re.findall(r"struct (\w+)", line)[0])
            continue

        if line.startswith("PROPERTY" ):
            for match in property_regex.finditer(line):
                attr_string = match.groups()
                print(f"\tattr_string={attr_string}")
            continue
    return structs

def main():
    f = open(R"C:\Users\dipao\source\repos\Phoenix-Engine\PhxLibs\src\PhxData\WorldComponents.def", "r")
    structs = parse_defines_header(f)

    # Generate C++ reflection table
#    for struct in structs.values():
#        print(f'const FieldInfo {struct["name"]}_Fields[] = {{')
#        for f in struct["fields"]:
#            attr_str = "{" + ", ".join(f'{{"{a[0]}", "{a[1]}"}}' for a in f["attrs"]) + "}"
#            print(f'  {{"{f["name"]}", offsetof({struct["name"]}, {f["name"]}), typeid({f["type"]}), nullptr, {attr_str}}},')
#        print('};\n')
    sys.exit(0)

if __name__ == "__main__":
    main()