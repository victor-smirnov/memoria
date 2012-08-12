#!/usr/bin/env python3
# -*- coding: utf-8 -*-

## This file is a part of the Memoria project.
## Copyright (C) 2012 Memoria team
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt or copy at
## http://www.boost.org/LICENSE_1_0.txt)

# This script is for postprocessing of Doxygen HTML output:
#   - removing of "inline" tag;
#   - linearization of methods' parameters.

# Usage: postprocess.py [-h|--help] [html_directory]
# All html files in html_directory will be processed.
# If html_directory isn't given, 'html' will be used.
#   -h|--help
#       Print help and exit.

import os, sys, re, codecs

def get_files_list(root_dir, ext_list):
    for (dir_name, subdirs, dir_files) in os.walk(root_dir, followlinks=True):
        for file_name in dir_files:
            (_, ext) = os.path.splitext(file_name)
            ext = ext[1:] #remove leading dot
            if ext in ext_list:
                rel_path = os.path.join(dir_name, file_name)
                yield rel_path

def remove_inline(directory):
    inline_re = re.compile(
        '\<\s*span\s+class\s*=\s*"mlabel"\s*\>\s*inline\s*\<\s*\/\s*span\s*\>',
        flags = re.I | re.M | re.S)

    for file_name in get_files_list(directory, ['html']):
        file = codecs.open(file_name, 'r', 'utf-8')
        lines = file.read()
        file.close()

        new_lines = inline_re.sub('', lines)

        file = codecs.open(file_name, 'w+', 'utf-8')
        file.write(new_lines)
        file.close()


def linearize_params(directory):
    get_op_tag_ptrn = lambda tag: '\<\s*{0}\s*\>'.format(tag)
    get_cl_tag_ptrn = lambda tag: '\<\s*\/\s*{0}\s*\>'.format(tag)
    re_flags = re.I | re.M | re.S
    memname_table_re = re.compile('<\s*table\s*class\s*=\s*"memname"\s*\>.*?'
        + get_cl_tag_ptrn('table'), re_flags)

    tr_re = re.compile(get_op_tag_ptrn('tr') + '.*?' + get_cl_tag_ptrn('tr'),
        re_flags)
    memname_re = re.compile('\<\s*td\s*class\s*=\s*"memname"\s*\>(.*?)'
        + get_cl_tag_ptrn('td'), re_flags)
    paramtype_re = re.compile('\<\s*td\s*class\s*=\s*"paramtype"\s*\>(.*?)'
        + get_cl_tag_ptrn('td'), re_flags)
    paramname_re = re.compile('\<\s*td\s*class\s*=\s*"paramname"\s*\>(.*?)'
        + get_cl_tag_ptrn('td'), re_flags)
    sp_begin_re = re.compile('^(&#160;|&nbsp;|\s)+', re_flags)
    sp_end_re = re.compile('(&#160;|&nbsp;|\s)+$', re_flags)
    simple_space_re = re.compile('\s+', re_flags)

    for file_name in get_files_list(directory, ['html']):
        file = codecs.open(file_name, 'r', 'utf-8')
        lines = file.read()
        file.close()

        new_lines = lines
        for m in reversed(list(memname_table_re.finditer(lines))):
            before = new_lines[:m.start()]
            inside = new_lines[m.start() : m.end()]
            after = new_lines[m.end():]

            memname = None
            params = []
            for (n, m_tr) in enumerate(tr_re.finditer(inside)):
                if n == 0:
                    m_memname = memname_re.search(m_tr.group(0))
                    if m_memname:
                        memname = m_memname.group(1).strip()

                paramtype = None
                paramname = None
                m_paramtype = paramtype_re.search(m_tr.group(0))
                if m_paramtype:
                    paramtype = m_paramtype.group(1)
                    paramtype = sp_begin_re.sub('', paramtype)
                    paramtype = sp_end_re.sub('', paramtype)
                    paramtype = paramtype.replace(',', '')
                    paramtype = simple_space_re.sub('&nbsp;', paramtype)

                m_paramname = paramname_re.search(m_tr.group(0))
                if m_paramname:
                    paramname = m_paramname.group(1)
                    paramname = sp_begin_re.sub('', paramname)
                    paramname = sp_end_re.sub('', paramname)
                    paramname = paramname.replace(',', '')
                    paramname = simple_space_re.sub('&nbsp;', paramname)

                if paramtype and paramname:
                    params.append( (paramtype, paramname) )

            if memname:
                inside = '<div class="memname">' \
                    + '<span class="memname">{0}</span> ( '.format(memname)
                params_strings = []
                for (paramtype, paramname) in params:
                    params_strings.append(
                        ('<span class="paramtype">{0}</span>'
                        + '&nbsp;<span class="paramname">{1}</span>')
                        .format(paramtype, paramname))
                inside += ', '.join(params_strings)
                inside += ' )'
            new_lines = before + inside + after

        file = codecs.open(file_name, 'w+', 'utf-8')
        file.write(new_lines)
        file.close()

def print_usage():
    print("Usage: postprocess.py [html_directory]")
    print("All html files in html_directory will be processed.")
    print("If html_directory isn't given, 'html' will be used.")
    print("  -h|--help")
    print("      Print help and exit.")

def main(argv):
    if len(argv) > 1:
        if len(argv) >= 3 or argv[1] == '-h':
            print_usage()
            exit()

    directory = argv[-1] if len(argv) > 1 else 'html'

    if not os.path.exists(directory):
        print("Directory '{0}' doesn't exist.".format(directory))
        exit()

    remove_inline(directory)
    linearize_params(directory)

if __name__ == "__main__":
    main(sys.argv)
