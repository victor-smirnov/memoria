#!/usr/bin/env python3

import xml.dom.minidom
import os, re, codecs

def getAllChildrenByTagName(node, tag):
    for child in node.childNodes:
        if child.nodeName == tag:
            yield child

def getFirstChildByTagName(node, tag):
    for child in node.childNodes:
        if child.nodeName == tag:
            return child
    return None


class Class:
    def __init__(self, xml_node):
        self.__name = getFirstChildByTagName(xml_node, "name").firstChild.nodeValue
        self.__refid = xml_node.attributes["refid"].value

    def get_name(self):
        return self.__name

    def get_refid(self):
        return self.__refid

    def __str__(self):
        return self.__name


class MemberParam:
    def __init__(self, xml_node):
        t = getFirstChildByTagName(xml_node, 'declname')
        self.__declname = t.firstChild.nodeValue if t else ''
        self.__type = getFirstChildByTagName(xml_node, 'type').firstChild.nodeValue

    def get_declname(self):
        return self.__declname

    def get_type(self):
        return self.__type


class Description:
    para_re = re.compile('^\<para\>(.*)\<\/para\>', re.I | re.M | re.S)
    emphasis_re = re.compile('\<emphasis\>(.*?)\<\/emphasis\>', re.I | re.M | re.S)
    ref_re = re.compile('\<ref.*?\>(.*?)\<\/ref\>', re.I | re.M | re.S)

    params_list_re = re.compile('\<parameterlist\s+kind\s*=\s*"param"\>(.*?)\<\/parameterlist\>', re.I | re.M | re.S)
    param_item_re = re.compile('\<parameteritem\>(.*?)\<\/parameteritem\>', re.I | re.M | re.S)
    param_name_re = re.compile('\<parametername\>(.*?)\<\/parametername\>', re.I | re.M | re.S)
    param_descr_re = re.compile('\<parameterdescription\>(.*?)\<\/parameterdescription\>', re.I | re.M | re.S)

    image_re = re.compile('\<image.*?name="(.*?)"\>.*?\<\/image\>', re.I | re.M | re.S)

    returns_re = re.compile('\<simplesect kind="return"\>(.*?)\<\/simplesect\>', re.I | re.M | re.S)

    descr_para_re = re.compile('\<para\>(.*?)\<\/para\>', re.I | re.M | re.S)

    def __init__(self, xml_node):
        title_node = getFirstChildByTagName(xml_node, 'title')
        self.__title = None if title_node == None else title_node.firstChild.nodeValue
        self.__paragraphs = []
        for para_node in getAllChildrenByTagName(xml_node, 'para'):
            para_str = para_node.toxml().strip()
            para_str = Description.para_re.sub(r'\g<1>', para_str)
            para_str = Description.emphasis_re.sub(r'//\g<1>//', para_str)
            para_str = Description.emphasis_re.sub(r'//\g<1>//', para_str)
            para_str = Description.ref_re.sub(r'**\g<1>**', para_str)
            para_str = Description.image_re.sub(r'{{\g<1>|\g<1>}}', para_str)
            if 'image' in para_str:
                pass

            params_str = None
            m = Description.params_list_re.search(para_str)
            if m:
                params_str = '**Parameters:**' + r'\\' + '\n'
                for item in Description.param_item_re.findall(m.group(1)):
                    n = Description.param_name_re.search(item).group(1).strip()
                    d = Description.param_descr_re.search(item).group(1).strip()
                    d = Description.descr_para_re.sub(r'\g<1>\n\n', d)
                    d = d.strip()
                    params_str += ('\u00a0'*4 + '//{0}//\u00a0\u2012\u00a0{1}' + r'\\' + '\n').format(n, d)
                params_str += '\n'
                pass

            returns_str = None
            m = Description.returns_re.search(para_str)
            if m:
                returns_str = '**Returns:**' + r'\\' + '\n'
                returns_str += Description.descr_para_re.sub('\u00a0'*4 + r'\g<1>' + '\n\n', m.group(1))
                pass

            if params_str or returns_str:
                para_str = ''
                if params_str:
                    para_str += params_str
                if returns_str:
                    para_str += returns_str

            self.__paragraphs.append(para_str)
        pass

    def get_title(self):
        return self.__title

    def get_paragraphs(self):
        return self.__paragraphs


class ClassMember:
    def __init__(self, xml_node):
        self.__kind = xml_node.attributes["kind"].value
        self.__name = getFirstChildByTagName(xml_node, 'name').firstChild.nodeValue
        self.__definition = getFirstChildByTagName(xml_node, 'definition').firstChild.nodeValue

        self.__params = [
            MemberParam(param_node)
            for param_node in getAllChildrenByTagName(xml_node, 'param') ]

        self.__protection = xml_node.attributes["prot"].value
        self.__static = xml_node.attributes["static"].value == 'yes'
        self.__const = xml_node.attributes["const"].value == 'yes'

        if self.__name == 'mergeWithRightSibling':
            pass

        briefdescription_node = getFirstChildByTagName(xml_node, 'briefdescription')
        self.__brief_description = None if briefdescription_node == None else Description(briefdescription_node)
        detaileddescription_node = getFirstChildByTagName(xml_node, 'detaileddescription')
        self.__detailed_description = None if detaileddescription_node == None else Description(detaileddescription_node)
        pass

    def get_kind(self):
        return self.__kind

    def get_name(self):
        return self.__name

    def get_definition(self):
        return self.__definition

    def get_params(self):
        return self.__params

    def get_protection(self):
        return self.__protection

    def is_static(self):
        return self.__static

    def is_const(self):
        return self.__const

    def get_brief_description(self):
        return self.__brief_description

    def get_detailed_description(self):
        return self.__detailed_description


def output_classes(classes, xml_dir, doc_dir):
    for cl in classes:
        filepath = os.path.join(xml_dir, '{0}.xml'.format(cl.get_refid()))
        xml_doc = xml.dom.minidom.parse(filepath)

        compounddef = getFirstChildByTagName(xml_doc.documentElement, 'compounddef')
        listofallmembers = getFirstChildByTagName(compounddef, 'listofallmembers')

        members = { member.attributes["id"].value : ClassMember(member)
                   for section in getAllChildrenByTagName(compounddef, 'sectiondef')
                   for member in getAllChildrenByTagName(section, 'memberdef') }

        file = codecs.open(os.path.join(doc_dir, '{0}.wiki'.format(cl.get_refid())), 'w+', 'utf-8')
        file.write('= class {0} =\n\n'.format(cl.get_name()))

        for id in members:
            member = members[id]

            prot_str = '' if member.get_protection() == 'public' else '//{0}//'.format(member.get_protection())
            static_str = '' if member.is_static() == False else '//static//'
            const_str = '' if member.is_const() == False else '//const//'
            file.write("  ".join([prot_str, static_str, const_str]))
            file.write('\n\n')

            params = member.get_params()
            file.write('{0}\u00a0(\u00a0'.format(member.get_definition()))
            for param in params:
                file.write((r'\\' + '\n' + '\u00a0'*4 + '{0}\u00a0{1}').format(param.get_type(), param.get_declname()))
            file.write('\u00a0)')

            brief_description = member.get_brief_description()
            if brief_description:
                if brief_description.get_title():
                    file.write('**{0}**\n'.format(brief_description.get_title()))
                for para in brief_description.get_paragraphs():
                    file.write('{0}\n\n'.format(para))
                file.write('\n')

            detailed_description = member.get_detailed_description()
            if detailed_description:
                if detailed_description.get_title():
                    file.write('**{0}**\n'.format(detailed_description.get_title()))
                for para in detailed_description.get_paragraphs():
                    file.write('{0}\n\n'.format(para))
                file.write('\n')

            file.write('----\n----\n')
        file.close()
    pass

def main():
    doc_root = 'Documentation.wiki'
    doc_dir = 'wiki/'
    xml_dir = 'xml'

    xml_doc = xml.dom.minidom.parse(os.path.join(xml_dir, 'index.xml'))
    classes = [ Class(node) for node in getAllChildrenByTagName(xml_doc.documentElement, "compound")
               if node.attributes["kind"].value == "class"]

    file = open(os.path.join(doc_dir, doc_root), 'w+')
    file.write('= Documentation =\n')
    file.write('== Classes ==\n')
    for cl in classes:
        file.write('* [[{0}|{1}]]\n'.format(cl.get_refid(), cl.get_name()))
    file.close()

    output_classes(classes, xml_dir, doc_dir)

if __name__ == "__main__":
    main()
