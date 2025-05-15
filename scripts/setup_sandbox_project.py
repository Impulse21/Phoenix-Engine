import os
import sys
import json


def main():
    script_path = os.path.abspath(__file__)
    
    project_path = os.path.join(os.path.dirname(script_path), "../.workspace/projects/sandbox")
    print(f"Project path is '{project_path}'")
    if not os.path.exists(project_path):
        print("Creating directory")
        os.makedirs(project_path)
    
    config_json = {}
    config_json['name'] = 'sandbox'
    with open('project.json', 'w', encoding='utf-8') as f:
        json.dump(config_json, f, ensure_ascii=False, indent=4)

    sys.exit(0)

if __name__ == "__main__":
    main()