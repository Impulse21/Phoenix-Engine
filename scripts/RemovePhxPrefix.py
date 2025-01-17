import os
import argparse

# Run example: .\RemovePhxPrefix.py ".\phxengine\include\phx\core\" "phx"

def remove_prefix_from_files(directory, prefix):
    for filename in os.listdir(directory):
        if filename.startswith(prefix):
            new_name = filename[len(prefix):]
            old_file = os.path.join(directory, filename)
            new_file = os.path.join(directory, new_name)
            os.rename(old_file, new_file)
            print(f'Renamed: {filename} -> {new_name}')
        else:
            print(f'Skipped: {filename} (does not start with prefix "{prefix}")')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Remove a specified prefix from all files in a directory.")
    parser.add_argument("directory", type=str, help="Path to the directory containing the files.")
    parser.add_argument("prefix", type=str, help="Prefix to remove from the file names.")

    args = parser.parse_args()

    # Call the function with arguments from the command line
    remove_prefix_from_files(args.directory, args.prefix)
