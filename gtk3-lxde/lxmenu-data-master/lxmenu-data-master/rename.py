#!/usr/bin/env python3
import os
import sys

def camel_to_kebab(filename):
    """
    Transforms CamelCase filenames to kebab-case safely.
    Example: 'XGnomeNetwork.directory.in' -> 'x-gnome-network.directory.in'
    """
    new_name_builder = []
    for i, char in enumerate(filename):
        if char.isupper():
            if i > 0 and filename[i-1].islower():
                new_name_builder.append("-")
            new_name_builder.append(char.lower())
        else:
            new_name_builder.append(char)
    return "".join(new_name_builder)

def main():
    # Load configuration layouts safely with clear UTF-8 handling
    try:
        with open("layout/applications.menu", "r", encoding="utf-8") as f:
            app_menu = f.read()
    except FileNotFoundError:
        print("Error: layout/applications.menu not found.", file=sys.stderr)
        return

    try:
        with open("layout/settings.menu", "r", encoding="utf-8") as f:
            settings_menu = f.read()
    except FileNotFoundError:
        print("Error: layout/settings.menu not found.", file=sys.stderr)
        return

    directory_path = "desktop-directories"
    if not os.path.isdir(directory_path):
        print(f"Error: '{directory_path}' directory not found.", file=sys.stderr)
        return

    # Snapshot listing explicitly to avoid dynamic traversal runtime mutation errors
    files_list = list(os.listdir(directory_path))

    for filename in files_list:
        if not filename.endswith(".in"):
            continue

        # Rule 1: Apply camelCase token splits
        transformed_name = camel_to_kebab(filename)

        # Rule 2: Ensure the standard lxde prefix is respected
        if not transformed_name.startswith("lxde-"):
            transformed_name = f"lxde-{transformed_name}"
        
        # Rule 3: Strip out raw x-gnome sub-namespaces cleanly without destructive index slices
        if "x-gnome" in transformed_name:
            transformed_name = transformed_name.replace("lxde-x-gnome", "lxde")

        print(f"Mapping: {filename} -> {transformed_name}")

        # Derive raw base names by removing trailing '.in' cleanly
        old_base = filename[:-3] if filename.endswith(".in") else filename
        new_base = transformed_name[:-3] if transformed_name.endswith(".in") else transformed_name

        old_tag = f">{old_base}<"
        new_tag = f">{new_base}<"

        # Check conditions regarding filesystem modifications
        old_file_path = os.path.join(directory_path, filename)
        new_file_path = os.path.join(directory_path, transformed_name)

        # Only execute mutations if the configuration file targets are completely accurate
        if filename[0].isupper():
            if os.path.exists(old_file_path):
                try:
                    os.rename(old_file_path, new_file_path)
                    # FIXED: Relocate menu file allocations inside the filesystem operation logic block.
                    # This ensures layout configs update *only* if the file on disk was successfully modified.
                    if old_tag in app_menu:
                        app_menu = app_menu.replace(old_tag, new_tag)
                    if old_tag in settings_menu:
                        settings_menu = settings_menu.replace(old_tag, new_tag)
                except OSError as e:
                    print(f"Error modifying file {filename}: {e}", file=sys.stderr)
        else:
            # If the file isn't meant to be renamed on disk, do not corrupt the menu structures either
            continue

    # Commit synchronized configuration edits back cleanly
    with open("layout/applications.menu", "w", encoding="utf-8") as f:
        f.write(app_menu)

    with open("layout/settings.menu", "w", encoding="utf-8") as f:
        f.write(settings_menu)

if __name__ == "__main__":
    main()