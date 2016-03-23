
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/core/tools/louds_tree.hpp>

#include <pugixml.hpp>

#include <iostream>
#include <vector>

using namespace memoria;
using namespace std;
using namespace pugi;



template <typename TreeType>
size_t createRandomLouds(TreeType& tree, size_t size, size_t max_children = 10)
{
    size_t nodes_count = 1;

    tree.appendUDS(1);

    size_t last_nodes = 1;

    while (nodes_count <= size)
    {
        size_t count = 0;

        for (size_t c = 0; c < last_nodes; c++)
        {
            size_t children = getRandom(max_children);

            if (last_nodes == 1 && children == 0)
            {
                while ((children = getRandom(max_children)) == 0);
            }

            if (nodes_count + children <= size)
            {
                tree.appendUDS(children);
                count += children;
                nodes_count += children;
            }
            else {
                goto exit;
            }
        }

        if (count > 0)
        {
            last_nodes = count;
        }
        else {
            break;
        }
    }

    exit:

    size_t remainder = 2 * (nodes_count + 1) + 1 - tree.size();

    cout<<"size "<<tree.size()<<" remainder "<<remainder<<endl;

    for (size_t c = 0; c < remainder; c++) tree.appendUDS(0);

    tree.reindex();

    return nodes_count;
}


template <typename T>
LoudsTree createLouds(vector<T>& degrees)
{
    LoudsTree tree;

    for (auto d: degrees)
    {
        tree.appendUDS(d);
    }

    tree.reindex();
    return tree;
}

struct NodeOrAttr {
    xml_node        node_;
    xml_attribute   attr_;

    NodeOrAttr(const xml_node& node): node_(node) {}
    NodeOrAttr(const xml_attribute& attr): attr_(attr) {}

    bool is_node() const
    {
        return node_.type() != xml_node_type::node_null;
    }
};

vector<NodeOrAttr> getChildren(const xml_node& node)
{
    vector<NodeOrAttr> children;

    for (xml_node child: node)
    {
        children.push_back(child);
    }

    for (xml_node::attribute_iterator attr = node.attributes_begin(); attr != node.attributes_end(); attr++)
    {
        children.push_back(*attr);
    }

    return children;
}


template <typename TreeType>
void createLouds(TreeType& tree, const xml_node& node)
{
    vector<NodeOrAttr> level;
    level.push_back(node);

    tree.appendUDS(1);

    while (level.size() > 0)
    {
        vector<NodeOrAttr> next_level;

        for (NodeOrAttr& child: level)
        {
            if (child.is_node())
            {
                vector<NodeOrAttr> children = getChildren(child.node_);

                next_level.insert(next_level.end(), children.begin(), children.end());

                tree.appendUDS(children.size());
            }
            else {
                tree.appendUDS(0);
            }
        }

        level = next_level;
    }
}

template <typename TreeType>
vector<size_t> getChildren(TreeType& tree, size_t nodeIdx)
{
    size_t first = tree.firstChild(nodeIdx);
    if (first != TreeType::END)
    {
        vector<size_t> children;
        size_t last = tree.lastChild(nodeIdx);

        for (size_t c = first; c <= last; c++)
        {
            children.push_back(c);
        }

        return children;
    }
    else {
        return vector<size_t>();
    }
}

template <typename TreeType>
void checkTreeStructures(TreeType& tree, size_t nodeIdx, const NodeOrAttr& node)
{
    vector<size_t> c1       = getChildren(tree, nodeIdx);

    if (node.is_node())
    {
        vector<NodeOrAttr> c2   = getChildren(node.node_);

        if (c1.size() == c2.size())
        {
            for (size_t c = 0; c < c1.size(); c++)
            {
                checkTreeStructures(tree, c1[c], c2[c].node_);
            }
        }
        else
        {
            throw "structure mismatch!: nodes";
        }
    }
    else {
        if (c1.size() != 0)
        {
            throw "structure mismatch!: attribute";
        }
    }
}


template <typename TreeType>
void traverseTree(TreeType& tree, size_t nodeIdx, size_t& count)
{
    count++;

    size_t child = tree.firstChild(nodeIdx);

    while (child != TreeType::END)
    {
        traverseTree(tree, child, count);
        child = tree.nextSibling(child);
    }
}



void traverseDOM(const xml_node& node, size_t& count)
{
    count++;

    for (xml_node::attribute_iterator iter = node.attributes_begin(); iter != node.attributes_end(); iter++)
    {
        count++;
    }

    for (xml_node child: node)
    {
        traverseDOM(child, count);
    }
}



int main()
{
//  const char* xmlFile = "/home/developer/workspace/memoria-build/unix/bin/sandbox/articles9.xml";
//
//  xml_document doc;
//
//  doc.load_file(xmlFile, parse_full);
//
//  LoudsTree tree;
//  createLouds(tree, doc);
//
//  cout<<"LOUDS Size = "<<tree.size()<<" bits"<<endl;
//
//  BigInt t0 = getTimeInMillis();
//
//  tree.reindex();
//
////    tree.dump(cout);
//
//  BigInt t1 = getTimeInMillis();
//
//  cout<<"ReindexTime: "<<FormatTime(t1 - t0)<<endl;
//
//  try {
//      checkTreeStructures(tree, 0, doc);
//      cout<<"LOUDS and XML trees match"<<endl;
//  }
//  catch (const char* msg) {
//      cout<<msg<<endl;
//  }
//
//  BigInt t2 = getTimeInMillis();
//
//  cout<<"DOM + LOUDS TraverseTime: "<<FormatTime(t2 - t1)<<endl;
//
//  size_t louds_count = 0;
//  traverseTree(tree, 0, louds_count);
//
//  BigInt t3 = getTimeInMillis();
//
//  cout<<"LOUDS TraverseTime: "<<FormatTime(t3 - t2)<<" nodes="<<louds_count<<endl;
//
//  size_t node_count = 0;
//  traverseDOM(doc, node_count);
//
//  BigInt t4 = getTimeInMillis();
//
//  cout<<"DOM TraverseTime: "<<FormatTime(t4 - t3)<<" nodes="<<node_count<<endl;


    LoudsTree tree;
    size_t count1 = createRandomLouds(tree, 100);

    tree.dump();

    size_t count2 = 0;
    traverseTree(tree, 0, count2);

    cout<<count1<<" "<<count2<<endl;

    vector<size_t> degrees1 = {1,4,2,2,2,2,0,0,2,1,0,0,2,0,0,0,1,1,0,0,0};

    LoudsTree tree1 = createLouds(degrees1);

    size_t count3 = 0;
    traverseTree(tree1, 0, count3);

    cout<<count3<<endl;

    tree1.traverseSubtree(13, [](size_t left, size_t right, size_t level) {
        cout<<left<<" "<<right<<endl;
    });


    vector<size_t> degrees2 = {1, 3, 1,3,1, 1,2,2,2,1, 1,0,0,0,0,0,0,1, 0,0};

    LoudsTree tree2 = createLouds(degrees2);

    tree2.dump();

    tree2.traverseSubtree(3, [](size_t left, size_t right, size_t level) {
        cout<<left<<" "<<right<<endl;
    });


    auto tree4 = tree2.getSubtree(3);

    tree4.dump();

    size_t count4 = 0;
    traverseTree(tree4, 0, count4);

    cout<<count4<<endl;

    return 0;
}
