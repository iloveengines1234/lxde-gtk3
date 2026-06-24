#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import argparse
import re
import sys
from pathlib import Path


def convert_man_to_cfg(man_file):
    """
    Convert a .man file to .cfg format.
    
    Parses lines with format "KEY    VALUE" and converts to "KEY=VALUE".
    Lines without the pattern are passed through unchanged.
    
    Args:
        man_file (Path): Path to the input .man file
        
    Returns:
        bool: True if successful, False otherwise
    """
    man_path = Path(man_file)
    cfg_path = man_path.with_suffix('.cfg')
    
    try:
        with open(man_path, 'r', encoding='utf-8') as fd_in, \
             open(cfg_path, 'w', encoding='utf-8') as fd_out:
            for input_line in fd_in:
                input_line = input_line.rstrip('\n')
                
                # Match pattern: non-whitespace characters, multiple spaces, then content
                match = re.match(r'^(\S+)\s{2,}(.+)$', input_line)
                
                if match:
                    output_line = f"{match.group(1)}={match.group(2)}\n"
                else:
                    output_line = f"{input_line}\n"
                
                fd_out.write(output_line)
        
        print(f"Successfully converted: {man_path} → {cfg_path}")
        return True
        
    except FileNotFoundError:
        print(f"ERROR: File not found: {man_path}", file=sys.stderr)
        return False
    except IOError as e:
        print(f"ERROR: Cannot read/write file: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Convert .man file to .cfg format"
    )
    parser.add_argument(
        "file_man",
        help="Path to .man file to convert to .cfg"
    )
    
    args = parser.parse_args()
    
    success = convert_man_to_cfg(args.file_man)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
