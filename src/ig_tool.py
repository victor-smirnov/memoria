#!/usr/bin/env python3
# -*- coding: utf-8 -*-
		
## Copyright 2012 Memoria team
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

# This script performs checks on include guards in source files
# and is able to rewrite it in accordance with file path.

import sys, os, re

#enum emulation
class ig_check_result:
	ok = 0
	not_found = 1
	broken = 2
	not_according = 3

#enum emulation
class directives:
	ifndef = 0
	define = 1


def get_files_list(root_dir, ext_list):
	files = []
	for (dir_name, subdirs, dir_files) in os.walk(root_dir, followlinks=True):
		for file_name in dir_files:
			(_, ext) = os.path.splitext(file_name)
			ext = ext[1:] #remove leading dot
			if ext in ext_list:
				rel_path = os.path.join(dir_name, file_name)
				files.append(rel_path)
	return files


def extract_directive(line, which):
	pattern = {
		directives.ifndef: r"^(\s*)#ifndef(\s+)(?P<name>.*?)\s*(//.*)?$",
		directives.define: r"^(\s*)#define(\s+)(?P<name>.*?)\s*(//.*)?$"}[which]
	match = re.match(pattern, line.strip())
	result = match.group("name") if match else None
	return result

	
def get_ig(file_name):
	#TODO make block comments more strict

	ifndef_name = None
	ifndef_line = None
	define_name = None
	define_line = None

	is_block_comment = False

	file = open(file_name, "r")
	try:
		for (line_num, line) in enumerate(file):
			# skip empty and commented lines
			if re.match(r"^\s*(//.*)?$", line): continue

			# deal with block comments
			if not is_block_comment:
				if re.match(r"^\s*/\*", line):
					is_block_comment = True
					continue
			else:
				if re.match(r".*\*/", line):
					is_block_comment = False
				continue

			# extract #ifndef
			if not ifndef_name:
				ifndef_name = extract_directive(line, directives.ifndef)
				if ifndef_name:
					ifndef_line = line_num
					continue 

			# extract #define
			if ifndef_name:
				define_name = extract_directive(line, directives.define)
				if define_name:
					define_line = line_num
					break

			# #define not detected
			break
	finally:
		file.close()
	return (ifndef_name, ifndef_line, define_name, define_line)


def get_all_igs(files):
	result = { fn : get_ig(fn) for fn in files }
	return result

	
def build_ig(root_dir, file_name):

	abs_file_name = os.path.abspath(file_name)
	common_prefix = os.path.commonprefix( [abs_file_name, os.path.abspath(root_dir)] )
	remainder = abs_file_name.replace(common_prefix, '', 1)

	path_elements = remainder.split(os.path.sep)
	path_elements = [ e.upper() for e in path_elements ]
	# replace dot with "_" in filename
	path_elements[-1] = path_elements[-1].replace('.', '_')
	ig = '_'.join(path_elements)
	return ig

	
def check(root_dir, igs):
	result = {}
	for key in igs:
		(ifndef_name, ifndef_line, define_name, define_line) = igs[key]
		result[key] = ig_check_result.ok
		if not ifndef_name and not define_name:
			result[key] = ig_check_result.not_found
		if not define_name:
			result[key] = ig_check_result.broken
		if ifndef_name != define_name:
			result[key] = ig_check_result.broken
		if ifndef_name != build_ig(root_dir, key):
			result[key] = ig_check_result.not_according
	return result
	

def print_check(root_dir, igs, is_abs_paths, is_sort):
	checks = check(root_dir, igs)
	file_names = list(checks.keys())
	if is_sort:
		file_names.sort()
	for file_name in file_names:
		c = checks[file_name]
		diplay_name = os.path.abspath(file_name) if is_abs_paths else file_name
		if c == ig_check_result.not_found:
			print('{0}:'.format(diplay_name))
			print('Include guards not found')
		elif c == ig_check_result.broken:
			print('{0}:'.format(diplay_name))
			print('Include guards is broken')
		elif c == ig_check_result.not_according:
			print('{0}:'.format(diplay_name))
			print('Include guards is incorrect')
			print('Current: "{0}"'.format(igs[file_name][0]))
			print('Correct: "{0}"'.format(build_ig(root_dir, file_name)))

		if c != ig_check_result.ok:
			print('')


def make_according(root_dir, igs, is_verbose, is_abs_paths, is_sort):
	checks = check(igs)
	not_according = [ i for i in checks if checks[i] == ig_check_result.not_according ]
	if is_sort:
		not_according.sort()
	for file_name in not_according:
		f_read = open(file_name, 'r+U', newline='')
		try:
			old_ig = igs[file_name][0]
			ig = build_ig(root_dir, file_name)
			ifndef_line = igs[file_name][1]
			define_line = igs[file_name][3]

			lines = list(f_read)
			lines[ifndef_line] = '#ifndef {0}{1}'.format(ig, f_read.newlines)
			lines[define_line] = '#define {0}{1}'.format(ig, f_read.newlines)

			f_read.seek(0)
			f_read.truncate()
			f_read.writelines(lines)
		finally:
			f_read.close()

		if is_verbose:
			diplay_name = os.path.abspath(file_name) if is_abs_paths else file_name
			print(diplay_name)
			print('Old include guard: {0}'.format(old_ig))
			print('New include guard: {0}'.format(ig))
			print('')


def print_usage():
		print(
"""This util is to check include guards in C++ source files and make them according to files paths
USAGE: {0} args root_dir
args:
  -a
    make according to files paths
  -v
    verbose mode (makes sense only with -a)
  -f
    print absolute paths
  -s
    sort paths

Default is simple checking.""".format(os.path.split(sys.argv[0])[1]))

			
def main():
	if len(sys.argv) < 2:
		print_usage()
		exit()

	ext_list = ['hpp', 'cpp']
	possible_args = [ '-v', '-a', '-f', '-s' ]
	is_args_fail = False
	
	is_repair = False
	is_verbose = False
	is_abs_paths = False
	is_sort = False
	for arg in sys.argv[1:-1]:
		if not arg in possible_args:
			print('Unknown argument {0}'.format(arg))
			is_args_fail = True
		if arg == '-v':
			is_verbose = True
		if arg == '-a':
			is_repair = True
		if arg == '-f':
			is_abs_paths = True
		if arg == '-s':
			is_sort = True
	if is_args_fail:
		exit()

	root_dir = sys.argv[-1]
	if not os.path.exists(root_dir):
		msg = "Directory '{0}' doesn't exist or inaccessible.".format(root_dir)
		print(msg)
		exit()

	if not os.path.isdir(root_dir):
		msg = "'{0}' isn't a directory.".format(root_dir)
		print(msg)
		exit()

	files = get_files_list(root_dir, ext_list)

	if not files:
		msg = "No *.cpp or *.hpp files have been found in directory '{0}' or it's subdirectories.".format(root_dir)
		print(msg)
		exit()

	igs = get_all_igs(files)
	if is_repair:
		make_according(igs, is_verbose, is_abs_paths, is_sort)
	else:
		print_check(root_dir, igs, is_abs_paths, is_sort)

#----
if __name__ == "__main__":
	main()
