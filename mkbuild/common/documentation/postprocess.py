#!/usr/bin/env python3
# -*- coding: utf-8 -*-

## This file is a part of the Memoria project.
## Copyright (C) 2012 Memoria team
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt or copy at
## http://www.boost.org/LICENSE_1_0.txt)

# This script is for postprocessing of Doxygen HTML output:
#   - removing of "inline" tag;
#   - linearization of methods' parameters;
#   - add links to sources in declaration table.

# Usage: postprocess.py [-h|--help] [html_directory]
# All html files in html_directory will be processed.
# If html_directory isn't given, 'html' will be used.
#   -h|--help
#       Print help and exit.

import os, sys, re, codecs

get_cl_tag_ptrn = lambda tag: '\<\s*\/\s*{0}\s*\>'.format(tag)

def get_op_tag_ptrn(tag, *attributes):
    result = '\<\s*{0}'.format(tag)
    for attr_name, attr_value in attributes:
        result += '\s+{0}\s*=\s*"{1}"'.format(attr_name, attr_value)
    result += '\s*\>'
    return result

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
        get_op_tag_ptrn('span', ('class', 'mlabel'))
        + '\s*inline\s*'
        + get_cl_tag_ptrn('span'), flags = re.I | re.M | re.S)

    for file_name in get_files_list(directory, ['html']):
        file = codecs.open(file_name, 'r', 'utf-8')
        lines = file.read()
        file.close()

        new_lines = inline_re.sub('', lines)

        file = codecs.open(file_name, 'w+', 'utf-8')
        file.write(new_lines)
        file.close()


def linearize_params(directory):
    re_flags = re.I | re.M | re.S
    memname_table_re = re.compile(
        get_op_tag_ptrn('table', ('class', 'memname'))
        + '.*?' + get_cl_tag_ptrn('table'), re_flags)

    tr_re = re.compile(get_op_tag_ptrn('tr') + '.*?' + get_cl_tag_ptrn('tr'),
        re_flags)
    memname_re = re.compile(
        get_op_tag_ptrn('td', ('class', 'memname'))
        + '(.*?)' + get_cl_tag_ptrn('td'), re_flags)
    paramtype_re = re.compile(
        get_op_tag_ptrn('td', ('class', 'paramtype'))
        + '(.*?)' + get_cl_tag_ptrn('td'), re_flags)
    paramname_re = re.compile(
        get_op_tag_ptrn('td', ('class', 'paramname'))
        + '(.*?)' + get_cl_tag_ptrn('td'), re_flags)
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


def add_links(directory):
    re_flags = re.I | re.M | re.S

    memitem_re = re.compile(
        # a open tag
        get_op_tag_ptrn('a', ('class', 'anchor'), ('id', '(?P<id>.+?)')) \
        # a close tag
        + '\s*' + get_cl_tag_ptrn('a') \
        # div class memitem open tag
        + '\s*' + get_op_tag_ptrn('div', ('class', 'memitem')) \
        # div class memdoc open tag
        + '.*?' + get_op_tag_ptrn('div', ('class', 'memdoc')) \
        # memdoc content
        + '(?P<memdoc>.*?)' \
        # div class memdoc close tag
        + '\s*' + get_cl_tag_ptrn('div') \
        # div class memitem close tag
        + '\s*' + get_cl_tag_ptrn('div'),
        flags = re_flags)
    
    srclink_re = re.compile(
        get_op_tag_ptrn('a', ('href', '(?P<url>.+?)'), ('class', 'srclink')),
        flags = re_flags)

    tr_re = re.compile(
        '(?P<opentag>'
        + get_op_tag_ptrn('tr', ('class', 'memitem:(?P<id>.+?)'))
        + ')'
        + '(?P<content>.*?)'
        + '(?P<closetag>' + get_cl_tag_ptrn('tr') + ')',
        flags = re_flags)

    memdesc_tr_re = re.compile(
        '(?P<opentag>'
        + get_op_tag_ptrn('tr', ('class', 'memdesc:.+?'))
        + ')'
        + '(?P<content>.*?)'
        + '(?P<closetag>' + get_cl_tag_ptrn('tr') + ')',
        flags = re_flags)
    
    mem_templ_params_re = re.compile(
        get_op_tag_ptrn('td', ('class', 'memTemplParams'))[:-5], re_flags)
    td_mdesc_right_re = re.compile(
        get_op_tag_ptrn('td', ('class', 'mdescRight')), re_flags)
    right_class_re = re.compile(
        '\s+class\s*=\s*"(?P<class>memTemplItemRight|memItemRight)"', re_flags)   
   
    colspan_re = re.compile('colspan\s*=\s*"(?P<colspan>\d+?)"', re_flags)
   
    for file_name in get_files_list(directory, ['html']):
        file = codecs.open(file_name, 'r', 'utf-8')
        lines = file.read()
        file.close()

        # collect links from memdocs
        links = {}
        for m in memitem_re.finditer(lines):
            id = m.group('id').strip()
            memdoc = m.group('memdoc').strip()
            srclink_m = srclink_re.search(memdoc)
            if not srclink_m:
                continue
            url = srclink_m.group('url').strip()
            links[id] = url
    
        new_lines = lines
    
        # update declaration table
        for m in reversed(list(tr_re.finditer(new_lines))):
            before = new_lines[:m.start()]
            inside = new_lines[m.start() : m.end()]
            after = new_lines[m.end():]
            
            id = m.group('id').strip()
            if id not in links.keys():
                continue
            
            content = m.group('content')
            # if not template row
            if not mem_templ_params_re.search(content):
                right_class_m = right_class_re.search(content)
                if right_class_m:
                    right_class = right_class_m.group('class')
                    content += '<td class="{1}" valing="bottom"><a href="{0}"><img src="source_link.png" height="16" width="16" title="View source code"/></a></td>'.format(links[id], right_class)
            else:
                colspan_m = colspan_re.search(content)
                if m:
                    colspan = int(colspan_m.group('colspan'))
                    content = colspan_re.sub('colspan="{0}"'.format(colspan+1), content)
                     
            new_lines = before + m.group('opentag') + content + m.group('closetag') + after

        # update memdesc
        for m in reversed(list(memdesc_tr_re.finditer(new_lines))):
            before = new_lines[:m.start()]
            inside = new_lines[m.start() : m.end()]
            after = new_lines[m.end():]

            content = m.group('content')
            if td_mdesc_right_re.search(content):
                content = td_mdesc_right_re.sub('<td class="mdescRight" colspan="2">', content)
                pass

            new_lines = before + m.group('opentag') + content + m.group('closetag') + after

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
    add_links(directory)

if __name__ == "__main__":
    main(sys.argv)
