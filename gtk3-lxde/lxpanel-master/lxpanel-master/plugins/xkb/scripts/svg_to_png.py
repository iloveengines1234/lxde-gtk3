#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import argparse
import sys
from pathlib import Path

import gi
gi.require_version('GdkPixbuf', '2.0')
from gi.repository import GdkPixbuf


def convert_svg_to_png(width, height, svg_dir=None):
    """Convert all SVG files in directory to PNG with specified dimensions."""
    
    if svg_dir is None:
        svg_dir = Path.cwd()
    else:
        svg_dir = Path(svg_dir)
    
    svg_files = sorted(svg_dir.glob("*.svg"))
    
    if not svg_files:
        print(f"No SVG files found in {svg_dir}", file=sys.stderr)
        return False
    
    for svg_file in svg_files:
        try:
            pixbuf = GdkPixbuf.Pixbuf.new_from_file(str(svg_file))
            new_pixbuf = pixbuf.scale_simple(width, height, GdkPixbuf.InterpType.HYPER)
            
            png_file = svg_file.with_suffix('.png')
            new_pixbuf.savev(str(png_file), "png", [], [])
            
            print(f"Converted: {svg_file.name} → {png_file.name}")
        except Exception as e:
            print(f"Error processing {svg_file.name}: {e}", file=sys.stderr)
            return False
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Convert SVG files to PNG with specified dimensions"
    )
    parser.add_argument("width", help="destination width in pixels", type=int)
    parser.add_argument("height", help="destination height in pixels", type=int)
    parser.add_argument(
        "--directory",
        "-d",
        help="directory containing SVG files (default: current directory)",
        default=None
    )
    
    args = parser.parse_args()
    
    if args.width <= 0 or args.height <= 0:
        parser.error("width and height must be positive integers")
    
    success = convert_svg_to_png(args.width, args.height, args.directory)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
