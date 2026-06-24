#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import argparse
import sys
from configparser import RawConfigParser
from pathlib import Path


def sort_config_file(cfg_file):
    """
    Sort sections and keys in a .cfg file alphabetically.
    
    Reads a ConfigParser-compatible .cfg file, sorts all sections
    and their keys alphabetically (case-insensitive), and writes
    the result to a .sorted file.
    
    Args:
        cfg_file (str or Path): Path to the input .cfg file
        
    Returns:
        bool: True if successful, False otherwise
    """
    cfg_path = Path(cfg_file)
    sorted_path = cfg_path.with_suffix('.cfg.sorted')
    
    try:
        # Read input config
        config_in = RawConfigParser()
        config_in.read(cfg_path, encoding='utf-8')
        
        # Create output config with sorted sections and keys
        config_out = RawConfigParser()
        
        for section in sorted(config_in.sections()):
            config_out.add_section(section)
            # Sort items by key (case-insensitive)
            sorted_items = sorted(
                config_in.items(section),
                key=lambda item: item[0].lower()
            )
            for key, value in sorted_items:
                config_out.set(section, key, value)
        
        # Write sorted config
        with open(sorted_path, 'w', encoding='utf-8') as fd_out:
            config_out.write(fd_out)
        
        print(f"Successfully sorted: {cfg_path} → {sorted_path}")
        return True
        
    except FileNotFoundError:
        print(f"ERROR: File not found: {cfg_path}", file=sys.stderr)
        return False
    except IOError as e:
        print(f"ERROR: Cannot read/write file: {e}", file=sys.stderr)
        return False
    except Exception as e:
        print(f"ERROR: Failed to parse config file: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Sort sections and keys in a .cfg file alphabetically"
    )
    parser.add_argument(
        "file_cfg",
        help="Path to .cfg file to sort"
    )
    
    args = parser.parse_args()
    
    success = sort_config_file(args.file_cfg)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
