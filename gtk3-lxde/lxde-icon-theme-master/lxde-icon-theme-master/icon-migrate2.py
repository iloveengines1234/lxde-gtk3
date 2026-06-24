#!/usr/bin/env python3
#
#       icon-migrate.py
#       
#       Copyright 2009 PCMan <pcman.tw@gmail.com>
#       
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 2 of the License, or
#       (at your option) any later version.
#       
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#       
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#       MA 02110-1301, USA.

import os
import sys
import shutil
import filecmp
import subprocess
from pathlib import Path
from xml.dom import minidom
from configparser import ConfigParser

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

required_gtk_version = (3, 24, 52)
loaded_gtk_version = (Gtk.get_major_version(), Gtk.get_minor_version(), Gtk.get_micro_version())
if loaded_gtk_version < required_gtk_version:
    print(
        f'Error: GTK {required_gtk_version[0]}.{required_gtk_version[1]}.{required_gtk_version[2]} ' \
        f'or newer is required. ' \
        f'Loaded GTK version is {loaded_gtk_version[0]}.{loaded_gtk_version[1]}.{loaded_gtk_version[2]}.'
        , file=sys.stderr)
    sys.exit(1)

icon_theme_dir = None
contexts = []

exts = ['.png', '.svg', '.xpm']

sizes = {}

class Mapping:
    def __init__(self, xml_icon_node):
        self.new_name = xml_icon_node.getAttribute('name')
        self.old_names = [link.childNodes[0].data for link in xml_icon_node.getElementsByTagName('link')]

class Context:
    def __init__(self, xml_context_node):
        self.name = xml_context_node.getAttribute('dir')
        self.mappings = [Mapping(icon_node) for icon_node in xml_context_node.getElementsByTagName('icon')]


def find_icon_file_in_context_dir(context_dir, icon_name):
    prefix = Path(context_dir)
    for ext in exts:
        candidate = prefix / f'{icon_name}{ext}'
        if candidate.exists():
            return candidate
    return None


def find_icon_file_in_all_contexts(icon_name):
    for dirs in sizes.values():
        for subdir in dirs:
            full_dir = Path(icon_theme_dir) / subdir
            found = find_icon_file_in_context_dir(full_dir, icon_name)
            if found:
                return found
    return None


def replace_link_with_real_file(path):
    path = Path(path)
    real_path = path.resolve()
    path.unlink()
    shutil.copy2(real_path, path)
    print(f'convert link to real file => copy {real_path} to {path}')


def convert_links_to_copies():
    for dirs in sizes.values():
        for subdir in dirs:
            directory = Path(icon_theme_dir) / subdir
            if not directory.is_dir():
                continue
            for entry in directory.iterdir():
                if entry.is_symlink():
                    replace_link_with_real_file(entry)


def is_icon_new_name(icon_name):
    return any(mapping.new_name == icon_name for context in contexts for mapping in context.mappings)


def convert_duplicated_files_to_symlinks():
    duplicates_file = Path(icon_theme_dir) / 'dups.txt'
    with duplicates_file.open('w', encoding='utf-8') as out:
        subprocess.run(['fdupes', '--nohidden', '-r', icon_theme_dir], stdout=out, check=True)

    files = []
    links = []

    print('\n\n--- svn commands ---\n')

    with duplicates_file.open('r', encoding='utf-8') as f:
        for line in f:
            item = line.strip()
            if item:
                files.append(item)
                continue
            if not files:
                continue

            files.sort(key=len)
            primary = next((file for file in files if is_icon_new_name(Path(file).stem)), files[0])
            print(f'svn --force add {primary}')

            for file in files:
                if file == primary:
                    continue
                print(f'svn --force rm {file}')
                file_path = Path(file)
                rel_path = os.path.relpath(primary, file_path.parent)
                links.append((rel_path, str(file_path)))
            files = []

    print('---------- symlinks ------------')
    for rel_path, dest in links:
        print(f'$(LN_S) -f {rel_path} $(DESTDIR)$(datadir)/icons/{dest}')


def find_icon_file_in_dir(directory, icon_name):
    directory = Path(directory)
    for ext in exts:
        candidate = directory / f'{icon_name}{ext}'
        if candidate.exists():
            return candidate, ext
    return None, None

symlinks = []
files_to_del = []

def choose_icon(new_name, icons):
    dlg = Gtk.Dialog(title=f'Choose an icon for {new_name}', flags=0)
    dlg.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
    content = dlg.get_content_area()
    content.set_spacing(6)

    label = Gtk.Label(label=f'Choose an icon for {new_name}')
    content.pack_start(label, False, False, 6)

    grid = Gtk.Grid(column_spacing=6, row_spacing=6, margin=6)
    content.pack_start(grid, True, True, 6)

    for idx, icon in enumerate(icons):
        button = Gtk.Button(label=Path(icon[0]).name)
        image = Gtk.Image.new_from_file(icon[0])
        button.set_image(image)
        button.set_always_show_image(True)
        button.connect('clicked', lambda *_args, response=idx, dialog=dlg: dialog.response(response))
        grid.attach(button, idx % 3, idx // 3, 1, 1)

    dlg.show_all()
    response = dlg.run()
    dlg.destroy()
    return response if response >= 0 else -1


def find_icon_file_of_size(size, icon_name):
    for sizedir in sizes.get(size, []):
        directory = Path(icon_theme_dir) / sizedir
        filename, ext = find_icon_file_in_dir(directory, icon_name)
        if filename:
            return filename, ext
    return None, None


def fix_icons_of_specified_size(size, subdir, mappings):
    print(f'fix icons in {subdir} of size {size}')
    target_dir = Path(icon_theme_dir) / subdir
    for mapping in mappings:
        print(f'new_name: {mapping.new_name} in {subdir}')
        fpath, ext = find_icon_file_in_dir(target_dir, mapping.new_name)
        if not fpath:  # icon with new_name is not found, choose one icon from old_names
            choices = []
            for old_name in mapping.old_names:
                for subdir2 in sizes.get(size, []):
                    directory = Path(icon_theme_dir) / subdir2
                    fpath2, ext2 = find_icon_file_in_dir(directory, old_name)
                    if fpath2:
                        choices.append((fpath2, ext2))
            if not choices:
                print(f'{mapping.new_name} is missing, need a new icon for it')
            elif len(choices) == 1:
                fpath2, ext2 = choices[0]
                fpath = target_dir / f'{mapping.new_name}{ext2}'
                shutil.copy2(fpath2, fpath)
            else:
                same = all(
                    choices[i][1] == choices[i+1][1] and filecmp.cmp(choices[i][0], choices[i+1][0], shallow=False)
                    for i in range(len(choices) - 1)
                )
                if same:
                    fpath2, ext2 = choices[0]
                    fpath = target_dir / f'{mapping.new_name}{ext2}'
                    shutil.copy2(fpath2, fpath)
                else:
                    print('choices:', choices)
                    ret = choose_icon(mapping.new_name, choices)
                    if ret >= 0:
                        fpath = target_dir / f'{mapping.new_name}{choices[ret][1]}'
                        shutil.copy2(choices[ret][0], fpath)
                    else:
                        print(f'{mapping.new_name} is missing, need a new icon for it')
        else:
            print(f'{mapping.new_name} is found: {fpath}')
            for old_name in mapping.old_names:
                fpath2, ext2 = find_icon_file_of_size(size, old_name)
                if not fpath2:
                    fpath2 = fpath.parent / f'{old_name}{ext}'
                    print(f'copy newname to oldname: {fpath} => {fpath2}')
                    shutil.copy2(fpath, fpath2)


# ----------------------------------------------------------------
# start


def main(argv=None):
    global icon_theme_dir
    if argv is None:
        argv = sys.argv
    if len(argv) < 2:
        print('Usage: icon-migrate2.py ICON_THEME_DIR', file=sys.stderr)
        return 1

    icon_theme_dir = Path(argv[1]).expanduser().resolve()
    if not icon_theme_dir.is_dir():
        print(f'Icon theme directory not found: {icon_theme_dir}', file=sys.stderr)
        return 1

    cfg = ConfigParser()
    cfg.read(icon_theme_dir / 'index.theme')
    subdirs = [subdir.strip() for subdir in cfg.get('Icon Theme', 'Directories').split(',') if subdir.strip()]

    for subdir in subdirs:
        size = cfg.getint(subdir, 'Size')
        sizes.setdefault(size, []).append(subdir)

    mapping_file = Path(__file__).resolve().parent / 'legacy-icon-mapping.xml'
    doc = minidom.parse(mapping_file)
    context_nodes = doc.getElementsByTagName('context')
    for context_node in context_nodes:
        contexts.append(Context(context_node))

    convert_links_to_copies()

    for size in sorted(sizes):
        print(f'for size: {size}')
        subdirs = sizes[size]
        for ctx in contexts:
            sub = None
            for subdir in subdirs:
                if Path(subdir).name == ctx.name:
                    sub = subdir
                    print(f'sub: {sub}')
                    break
            if not sub:
                sub = ctx.name
                print(f'missing context: {sub}')

            fix_icons_of_specified_size(size, sub, ctx.mappings)

    print('\n\n--------------------symlinks----------------------\n\n')
    convert_duplicated_files_to_symlinks()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
