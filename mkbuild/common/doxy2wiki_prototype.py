#!/usr/bin/env python3

import xml.dom.minidom
import os

class Class:
    def __init__(self, name, refid):
        self.__name = name
        self.__refid = refid

    def get_name(self):
        return self.__name

    def get_refid(self):
        return self.__refid

    def __str__(self):
        return self.__name


class ClassMember:
    def __init__(self, kind, name):
        self.__kind = kind
        self.__name = name

    def get_kind(self):
        return self.__kind

    def get_name(self):
        return self.__name


def getAllChildrenByTagName(node, tag):
    for child in node.childNodes:
        if child.nodeName == tag:
            yield child

def getFirstChildByTagName(node, tag):
    for child in node.childNodes:
        if child.nodeName == tag:
            return child
    return None

def output_classes(classes, xml_dir, doc_dir):
    for cl in classes:
        filepath = os.path.join(xml_dir, '{0}.xml'.format(cl.get_refid()))
        xml_doc = xml.dom.minidom.parse(filepath)

        compounddef = getFirstChildByTagName(xml_doc.documentElement, 'compounddef')
        listofallmembers = getFirstChildByTagName(compounddef, 'listofallmembers')

        members = { member.attributes["id"].value :
                   ClassMember(member.attributes["kind"].value, getFirstChildByTagName(member, 'name').firstChild.nodeValue)
                   for section in getAllChildrenByTagName(compounddef, 'sectiondef')
                   for member in getAllChildrenByTagName(section, 'memberdef') }

        file = open(os.path.join(doc_dir, '{0}.wiki'.format(cl.get_refid())), 'w+')
        file.write('= class {0} =\n'.format(cl.get_name()))

        for id in members:
            member = members[id]
            file.write('* {0} **{1}**\n'.format(member.get_kind(), member.get_name()))
        file.close()
    pass

def main():
    doc_root = 'Documentation.wiki'
    doc_dir = '../mywiki/wiki/'
    xml_dir = 'xml'

    xml_doc = xml.dom.minidom.parse(os.path.join(xml_dir, 'index.xml'))
    classes = [ Class(getFirstChildByTagName(node, "name").firstChild.nodeValue, node.attributes["refid"].value)
               for node in getAllChildrenByTagName(xml_doc.documentElement, "compound")
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
