#!/usr/bin/python3
# -*- coding: utf-8 -*-

## This file is part of the Memoria project.
## Copyright (C) 2011 Ivan Yurchenko <ivanhoe12@gmail.com>
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt or copy at
## http://www.boost.org/LICENSE_1_0.txt)

# This script performs various checks on include guards in source files.

import sys, os, re

def check_ext(filename, ext_list):
	result = False
	for ext in ext_list:
		if filename.endswith(ext):
			return True
	return result


def get_files_list(root_dir, ext_list):
	files = []
	for (dir_name, subdirs, files_in_dir) in os.walk(root_dir, followlinks=True):
		for f in files_in_dir:
			if check_ext(f, ext_list):
				rel_path = os.path.join(dir_name, f)
				files.append(os.path.abspath(rel_path))
	return files


def detect(line, which):
	pattern = {
		"ifndef": r"^#ifndef(\s+)(?P<name>.*)$",
		"define": r"^#define(\s+)(?P<name>.*)$"}[which]
	m = re.match(pattern, line.strip())
	if m:
		return (True, m.group("name"))
	else:
		return (False, None)


def get_inc_grds(files):
	result = []
	for f in files:
		ifndef = False
		ifndef_line = -1
		ifndef_name = ""

		define = False
		define_line = -1
		define_name = ""

		file = open(f, "r")
		try:
			current_line = -1
			for line in file:
				current_line += 1

				# detect #ifndef
				if not ifndef:
					(ifndef, ifndef_name) = detect(line, "ifndef")
					if ifndef:
						ifndef_line = current_line
						continue 
				
				# skip empty lines
				#compile it
				m = re.match(r"^\s*$", line)
				if m: continue

				# detect #define
				if ifndef:
					(define, define_name) = detect(line, "define")
					if define:
						define_line = current_line
						break

				# #define not detected
				# reset ifndef search
				ifndef = False

		finally:
			file.close()

		result.append( (f, ifndef_line, ifndef_name, define_line, define_name) )

	return result


def check_inc_grds(inc_grds, exit_on_error=False, check_existence=False, check_incorrectness=True, verbose=False):
	result = True
	for f in inc_grds:
		(file_name, ifndef_line, ifndef_name, define_line, define_name) = f

		if check_existence \
			and ifndef_line == -1 and define_line == -1:
			if verbose: print (file_name + ': ' + "Include guard doesn't exist")
			result = False
			if exit_on_error: return result
			continue

		if check_incorrectness \
			and ifndef_line != -1 and define_line == -1 and ifndef_name != define_name:
			print (ifndef_line)
			if verbose: print (file_name + ': ' + "Incorrect include guard")
			result = False
			if exit_on_error: return result
			continue
	return result


def find_duplications(inc_grds):
	result = []
	inc_grds_names = [( i[0], i[2] ) for i in inc_grds if i[1] != -1 and i[3] != -1 ]
	while len(inc_grds_names) > 0:
		a = inc_grds_names[0]
		dupls = [ d for d in inc_grds_names if d[1] == a[1] ]
		inc_grds_names = [ d for d in inc_grds_names if d[1] != a[1] ]
		if len(dupls) > 1:
			result.append(dupls)
	return result


def check_duplications(inc_grds):
	dupls = find_duplications(inc_grds)
	if len(dupls) == 0:
		return True

	print("Include guards duplications have been found:")
	print()
	for d in dupls:
		print ("\"" + d[0][1] + "\":")
		for i in d:
			print(i[0])
		print()


def main():
	ext_list = ["hpp", "cpp"]

	if len(sys.argv) != 2:
		usage = "USAGE: {0} root_dir".format(os.path.split(sys.argv[0])[1])
		print(usage)
		exit()

	root_dir = sys.argv[1]
	if not os.path.exists(root_dir):
		msg = "Directory '{0}' doesn't exist or inaccessible.".format(root_dir)
		print(msg)
		exit()

	if not os.path.isdir(root_dir):
		msg = "'{0}' isn't a directory.".format(root_dir)
		print(msg)
		exit()

	files = get_files_list(root_dir, ext_list)

	if len(files) == 0:
		msg = "No *.cpp or *.hpp files have been found in directory '{0}' or it's subdirectories.".format(root_dir)
		print(msg)
		exit()

	inc_grds = get_inc_grds(files)
	#check_inc_grds(inc_grds, check_existence=True, verbose=True)
	check_duplications(inc_grds)

#----
if __name__ == "__main__":
	main()
