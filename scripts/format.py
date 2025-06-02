import subprocess
import sys
import argparse
from pathlib import Path

#!/usr/bin/env python3
"""
Format all C and H files using clang-format.
"""


def find_c_h_files(directory, recursive=True):
    """Find all .c and .h files in the given directory."""
    if recursive:
        c_files = list(Path(directory).glob("**/*.c"))
        h_files = list(Path(directory).glob("**/*.h"))
    else:
        c_files = list(Path(directory).glob("*.c"))
        h_files = list(Path(directory).glob("*.h"))
    return c_files + h_files


def format_file(file_path, style="file"):
    """Format a single file using clang-format."""
    try:
        subprocess.run(
            ["clang-format", f"-style={style}", "-i", str(file_path)], check=True
        )
        print(f"Formatted: {file_path}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error formatting {file_path}: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Format C and H files using clang-format"
    )
    parser.add_argument(
        "directory", nargs="?", default=".", help="Directory to search for files"
    )
    parser.add_argument(
        "--style", default="file", help="Formatting style (default: file)"
    )
    parser.add_argument(
        "--non-recursive", action="store_true", help="Don't search recursively"
    )
    args = parser.parse_args()

    files = find_c_h_files(args.directory, not args.non_recursive)

    if not files:
        print(f"No C or H files found in {args.directory}")
        return 1

    print(f"Found {len(files)} files to format")

    success_count = 0
    for file_path in files:
        if format_file(file_path, args.style):
            success_count += 1

    print(f"Successfully formatted {success_count} of {len(files)} files")
    return 0 if success_count == len(files) else 1


if __name__ == "__main__":
    sys.exit(main())
