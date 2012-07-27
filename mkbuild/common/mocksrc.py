#!/usr/bin/env python3

import sys, os, re


class DebugInfoEntry:
    class_tag_re = re.compile('DW_TAG_class_type', re.I)
    level_num_tag_re = re.compile('^\<(?P<level>\d+?)\>\<(?P<id>.+?)\>\<DW_TAG_(?P<tag>\w+?)\>', re.I)
    name_re = re.compile('DW_AT_name\<\"(?P<name>.+?)\"\>', re.I)
    linkage_name_re = re.compile('DW_AT_MIPS_linkage_name\<\"(?P<lnkname>.+?)\"\>', re.I)

    # TODO what if there are spaces in the path? test and fix
    decl_file_re = re.compile('DW_AT_decl_file\<0x\w+?\s(?P<path>.+?)\>', re.I)
    decl_line_re = re.compile('DW_AT_decl_line\<(?P<linenum>.+?)\>', re.I)

    def __init__(self, line, parent):
        self.__line = line
        self.__children = []
        m = DebugInfoEntry.level_num_tag_re.match(line)
        self.__level = int(m.group('level'))
        self.__id = m.group('id')
        self.__tag = m.group('tag')

        self.__declaration = None
        self.__typedef = None

        if parent:
            self.__parent = parent
            parent.add_child(self)
        else:
            self.__parent = None
        pass

    def get_level(self):
        return self.__level

    def get_str_id(self):
        return self.__id

    def get_int_id(self):
        return int(self.__id, 16)

    def get_parent(self):
        return self.__parent

    def get_line(self):
        return self.__line

    def get_tag(self):
        return self.__tag

    def add_child(self, child):
        self.__children.append(child)
        pass

    def get_children(self):
        return self.__children

    def get_decl_file(self):
        m = DebugInfoEntry.decl_file_re.search(self.__line)
        if m:
            return m.group('path')
        return None

    def get_decl_line(self):
        m = DebugInfoEntry.decl_line_re.search(self.__line)
        if m:
            return int(m.group('linenum'), 16)
        return None

    def wrap_declaration(self, decl):
        self.__declaration = decl

    def get_name(self):
        line = self.__line
        if self.__declaration:
            line = self.__declaration.__line

        m = DebugInfoEntry.name_re.search(line)
        if m:
            return m.group('name')
        if self.__typedef:
            typedef_name = self.__typedef.get_name()
            if typedef_name:
                return typedef_name
        m = DebugInfoEntry.linkage_name_re.search(line)
        if m:
            return m.group('lnkname')
        return None

    def set_typedef(self, td):
        self.__typedef = td


class Class:
    name_re = re.compile('DW_AT_name\<\"(?P<name>.+?)\"\>', re.I)
    specification_re = re.compile('DW_AT_specification\<\<(?P<spid>.+?)\>\>', re.I)
    inheritance_re = re.compile('DW_TAG_inheritance', re.I)
    inheritance_type_re = re.compile('DW_AT_type\<\<(?P<id>.+?)\>\>', re.I)

    def __init__(self, entry):
        self.__entry = entry
        self.__sp = None
        self.__parents = []
        self.__children = []
        self.__methods = None

    def get_entry(self):
        return self.__entry

    def get_name(self):
        return self.__entry.get_name()

    def set_specification(self, sp):
        self.__sp = sp

    def get_specification(self):
        return self.__sp

    def get_parents_ids(self):
        result = []
        for entry in self.__entry.get_children():
            if Class.inheritance_re.search(entry.get_line()):
                id_str = Class.inheritance_type_re.search(entry.get_line()).group('id')
                result.append(int(id_str, 16))
        return result

    def add_parent(self, parent):
        self.__parents.append(parent)
        parent.__children.append(self)

    def get_parents(self):
        return self.__parents

    def get_children(self):
        return self.__children

    def get_methods(self):
        if not self.__methods:
            self.__methods = []
            for child_entry in self.__entry.get_children():
                if child_entry.get_tag() == 'subprogram':
                    method = Method(child_entry, self)
                    self.__methods.append(method)
        return self.__methods

    def __str__(self):
        return self.get_name()


class Method:
    #artificial_re = re.compile('DW_AT_artificial\<yes\(1\)\>', re.I)
    def __init__(self, entry, klass):
        self.__entry = entry
        self.__class = klass

    def get_name(self):
        return self.__entry.get_name()

    def get_decl_file(self):
        return self.__entry.get_decl_file()

    def get_decl_line(self):
        return self.__entry.get_decl_line()


def collect_debug_info(source, filter_func = None):
    if not filter_func:
        filter_func = lambda x: True

    result = []
    is_debug_info = False
    is_accepted = False

    for line in source:
        if not is_debug_info:
            if line.startswith('.debug_info'):
                di_lines = [line]
                is_accepted = filter_func(line)
                is_debug_info = True
                is_accepted = False
        else:
            if line.startswith('.debug_info'):
                if is_accepted:
                    result.append(di_lines)
                di_lines = [line]
                is_accepted = filter_func(line)
            elif line.startswith('.'):
                if is_accepted:
                    result.append(di_lines)
                is_debug_info = False
            else:
                di_lines.append(line)
                is_accepted = is_accepted or filter_func(line)
    if is_debug_info and is_accepted:
        result.append(di_lines)
    return result


def process_debug_info(debug_info):
    entries = {}
    specifications = {}
    typedefs = {}

    line_re = re.compile('^\<(?P<level>\d+?)\>', re.I)
    specification_re = re.compile('DW_AT_specification\<\<(?P<spid>.+?)\>\>', re.I)
    type_re = re.compile('DW_AT_type\<\<(?P<typenum>.+?)\>\>', re.I)

    parents_for_level = { 0: None }
    for line in debug_info:
        m = line_re.match(line)
        if m:
            level = int(m.group('level'))
            entry = DebugInfoEntry(line, parents_for_level[level])
            parents_for_level[level+1] = entry

            if entry.get_tag() == 'class_type' or entry.get_tag() == 'structure_type':
                entries[entry.get_int_id()] = entry

                spec_m = specification_re.search(line)
                if spec_m:
                    spid = int(spec_m.group('spid'), 16)
                    specifications[spid] = entry
            if entry.get_tag() == 'typedef':
                type_m = type_re.search(line)
                if type_m:
                    typenum = int(type_m.group('typenum'), 16)
                    typedefs[typenum] = entry

    # wrap specifications into declarations
    for spid in specifications:
        entry = specifications[spid]
        declaration = entries[spid]
        entry.wrap_declaration(declaration)

    # attache typedefs to entries
    for id in entries:
        entry = entries[id]
        id = entry.get_int_id()
        if id in typedefs:
            typedef = typedefs[id]
            entry.set_typedef(typedef)


    return (entries, specifications)

def build_class_hiererchy(debug_info_entries, specifications):
    classes = {}

    for id in debug_info_entries:
        if id in specifications:
            id = specifications[id].get_int_id()
        use_entry = debug_info_entries[id]
        tag = use_entry.get_tag()
        if tag in ['class_type', 'structure_type']:
            classes[id] = Class(use_entry)

    for id in classes:
        c = classes[id]
        for pid in c.get_parents_ids():
            if pid in specifications:
                pid = specifications[pid].get_int_id()
            c.add_parent(classes[pid])

    return classes


def find_ctr_class(classes):
    class_name_re = re.compile('^Ctr\<memoria::CtrTypesT\<memoria::CtrTF\<memoria::[^\<\>:,]+\<\>, memoria::[^\<\>:,]+, memoria::Vector\>::Types\>\s*\>', re.I)
    for id in classes:
        c = classes[id]
        m = class_name_re.match(classes[id].get_name())
        if m:
            return c

def find_iter_class(classes):
    class_name_re = re.compile('^Iter\<memoria::BTreeIterTypes\<memoria::IterTypesT\<memoria::CtrTF\<memoria::SmallProfile\<\>, memoria::DynVector, memoria::Vector\>::Types\> \> \>', re.I)
    for id in classes:
        c = classes[id]
        m = class_name_re.match(classes[id].get_name())
        if m:
            return c


def get_linear_hierarchy(klass):
    result = [klass]
    current = klass
    while (len(current.get_parents()) > 0):
        result.append(current.get_parents()[0])
        current = current.get_parents()[0]
    return result


def collect_methods(linear_hierarchy):
    result = {}
    for klass in reversed(linear_hierarchy):
        for method in klass.get_methods():
            if method.get_decl_file():
                result[method.get_name()] = method
    return result


def extract_signature(lines, line_num):
    result = ''

    # zero-based indexing adjustment
    line_num -= 1

    # first pass - forward until '{'
    current_line_num = line_num
    current_line = lines[current_line_num]
    position = 0

    if current_line.startswith('MEMORIA_'):
        return '!! MACRO: ' + current_line.strip()

    while True:
        symbol = current_line[position]
        if symbol == '{':
            result += '{}'
            break
        if symbol == ';':
            result += '{}'
            break

        result += symbol
        position += 1
        if symbol == '\n':
            current_line_num += 1
            current_line = lines[current_line_num]
            position = 0

    # some kind of automaton

    # states:
    state_initial = 0 # initial state
    state_brace_passed = 1 # after { of function's body
    state_block_comment = 2 # inside block comment /* */
    state_line_comment = 3 # single line comment //
    state_comment_passed = 4 # after every comment
    state_end = 5 # final state
    current_state = state_initial

    current_line_num = line_num - 1
    while current_state != state_end:
        current_line = lines[current_line_num]
        s_current_line = current_line.strip()
        if len(s_current_line) == 0:
            current_line_num -= 1
            continue

        if current_state == state_initial:
            #if s_current_line.endswith('{'):
            current_state = state_brace_passed
            continue
            pass

        elif current_state == state_brace_passed:
            if s_current_line.endswith('*/'):
                current_state = state_block_comment
            elif s_current_line.startswith('//'):
                current_state = state_line_comment
            elif s_current_line.startswith('#define') or s_current_line[-1] in [ '{', '}', ';', ':']:
                current_state = state_end
                continue
            pass

        elif current_state == state_block_comment:
            if s_current_line.startswith('/*'):
                current_state = state_comment_passed
            pass

        elif current_state == state_line_comment:
            if s_current_line.startswith('#define') or s_current_line[-1] in ['{', '}', ';', ':']:
                current_state = state_end
                continue
            pass

        elif current_state == state_comment_passed:
            if s_current_line.endswith('*/'):
                current_state = state_block_comment
            elif s_current_line.startswith('//'):
                current_state = state_line_comment
            else:
                current_state = state_end
                continue
            pass

        result = current_line + result
        current_line_num -= 1

    result = result.rstrip()
    if result.endswith('}'):
        pass
    elif result.endswith('{'):
        result += '}'
    else:
        result += '{}'

    result = result.replace('M_PARAMS', '').replace('M_TYPE::', '')
    return result


def do_output(ctr_methods, iter_methods, output_dir, is_just_print_places):
    if is_just_print_places:
        for m in ctr_methods:
            decl_file = ctr_methods[m].get_decl_file()
            decl_line = ctr_methods[m].get_decl_line()
            print(m)
            print('[{0}:{1}]'.format(decl_file, decl_line))
            print()
        return

    out_file = open(os.path.join(output_dir, 'output.cpp'), 'w')
    out_file.write('class Vector<SimpleProfile>\n')
    out_file.write('{\n')
    out_file.write('public:\n')

    out_file.write('    class Iterator<SimpleProfile>\n')
    out_file.write('    {\n')
    out_file.write('    public:\n')
    for m in iter_methods:
        decl_file = iter_methods[m].get_decl_file()
        decl_line = iter_methods[m].get_decl_line()

        f = open(decl_file)
        lines = f.readlines()
        f.close()

        for line in extract_signature(lines, decl_line).split('\n'):
            sl = line.strip()
            if sl.startswith('!!'):
                sl = '//' + sl
            out_file.write('        ' + sl + '\n')
        out_file.write('\n')
    out_file.write('    }\n\n')

    for m in ctr_methods:
        decl_file = ctr_methods[m].get_decl_file()
        decl_line = ctr_methods[m].get_decl_line()

        f = open(decl_file)
        lines = f.readlines()
        f.close()

        for line in extract_signature(lines, decl_line).split('\n'):
            sl = line.strip()
            if sl.startswith('!!'):
                sl = '//' + sl
            out_file.write('    ' + sl + '\n')
        out_file.write('\n')

    out_file.write('}\n')
    out_file.close()


def print_usage(script_name):
    print('Usage:')
    print('{0} [-o <output_dir>] [-p]'.format(script_name))

def main(argv):
    # setting up
    script_name = os.path.basename(argv[0])

    output_path = None
    is_just_print_places = False
    i = 1
    while i < len(argv):
        if argv[i] == '-o' and i+1 < len(argv):
            output_path = argv[i+1]
            i += 2
        elif argv[i] == '-p':
            is_just_print_places = True
            i += 1
        else:
            print_usage(script_name)
            exit()

    # main part
    if sys.stdin.isatty():
        print("No input data!")
        exit()

    ctr_class_name_re = re.compile('"Ctr\<memoria::CtrTypesT\<memoria::CtrTF\<memoria::[^\<\>:,]+\<\>, memoria::[^\<\>:,]+, memoria::Vector\>::Types\>\s*\>', re.I)
    debug_info = collect_debug_info(sys.stdin, lambda x: ctr_class_name_re.search(x))
    if len(debug_info) > 1:
        print('More than one suitable .debug_info, first is used.')
    if len(debug_info) == 0:
        sys.stderr.write('No suitable .debug_info, exit.\n')
        exit()

    (entries, specifications) = process_debug_info(debug_info[0])
    classes = build_class_hiererchy(entries, specifications)
    ctr_class = find_ctr_class(classes)
    linear_hierarchy = get_linear_hierarchy(ctr_class)
    ctr_methods = collect_methods(linear_hierarchy)

    iter_class = find_iter_class(classes)
    linear_hierarchy = get_linear_hierarchy(iter_class)
    iter_methods = collect_methods(linear_hierarchy)

    do_output(ctr_methods, iter_methods, output_path, is_just_print_places)
    
if __name__ == "__main__":
    main(sys.argv)
