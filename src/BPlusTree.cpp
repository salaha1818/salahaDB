#define BPLUSTREE_H

#include <vector>
#include <optional>
#include "PageManager.h"
#include "Database.h"
#include "BPlusTree.h"
#include <algorithm>

std::optional<RecordLocation> BPlusTree::search(int key) const {   
    Node* leaf = findLeaf(key);
    const std::vector<int>& keys = leaf->keys;
    for(int i = 0; i < keys.size(); i++){
        if(keys[i] == key){
            return leaf->values[i];
        }
    } 
    return std::nullopt;
}

void BPlusTree::insert(int key, const RecordLocation& location) {
    Node* leaf = findLeaf(key);
    std::vector<int>& keys = leaf->keys;
    std::vector<RecordLocation>& values = leaf->values;
    size_t i = 0; 

    auto it = std::find(keys.begin(), keys.end(), key);

    if (it == keys.end()) {
        while (i < keys.size() && keys[i] < key) {
            i++;
        }

        keys.insert(keys.begin() + i, key);
        values.insert(values.begin() + i, location);
    }

    if(leaf->keys.size() > MAX_KEYS) {
        splitLeaf(leaf);
    }
}

void BPlusTree::deleteTree(Node* node) {
    if (node == nullptr) {
        return;
    }

    for (Node* child : node->children) {
        deleteTree(child);
    }

    delete node;
}

BPlusTree::Node* BPlusTree::findLeaf(int key) const {
    Node* node = root;

    while (!node->isLeaf) {
        size_t i = 0;

        while (i < node->keys.size() &&
               key >= node->keys[i]) {
            i++;
        }

        node = node->children[i];
    }

    return node;
}

void BPlusTree::splitLeaf(Node* leaf) {
    Node* newLeaf = new Node(true);
    size_t splitIndex = leaf->keys.size() / 2;
    std::vector<int>& keys = leaf->keys;
    std::vector<RecordLocation>& values = leaf->values;

    // seperate higher keys 
    newLeaf->keys.assign(keys.begin() + splitIndex, keys.end());

    // seperate higher values 
    newLeaf->values.assign(values.begin() + splitIndex, values.end());

    // erase the copied values
    keys.erase(keys.begin() + splitIndex, keys.end());
    values.erase(values.begin() + splitIndex, values.end());

    // connect the leaf linked list
    newLeaf->nextLeaf = leaf->nextLeaf;
    leaf->nextLeaf = newLeaf;

    // connect leaf to the same parent as the old one
    newLeaf->parent = leaf->parent;

    // find where the right leaf begins
    int separatorKey = newLeaf->keys[0];

    // if we start at root, need to create a internal node
    if (leaf == root) {
        Node* newRoot = new Node(false);

        // set values for node
        newRoot->keys.push_back(separatorKey);
        newRoot->children.push_back(leaf);
        newRoot->children.push_back(newLeaf);

        // set parents
        leaf->parent = newRoot;
        newLeaf->parent = newRoot;

        root = newRoot;
    } else {
        insertIntoParent(leaf, separatorKey, newLeaf);
    }
}

void BPlusTree::splitInternal(Node* node) {
    Node* newInternal = new Node(false);
    size_t splitIndex = node->keys.size() / 2;
    std::vector<int>& keys = node->keys;
    std::vector <Node*>& children = node -> children;
    
    // find which the key to move to parent
    int separatorKey = node->keys[splitIndex];

    // seperate higher keys 
    newInternal->keys.assign(keys.begin() + splitIndex + 1, keys.end());

    // seperate higher children 
    newInternal->children.assign(children.begin() + splitIndex + 1, children.end());

    // update parent for children
    for (Node* child : newInternal->children) {
        child->parent = newInternal;
    }

    // erase the copied keys and children
    keys.erase(keys.begin() + splitIndex , keys.end());
    children.erase(children.begin() + splitIndex + 1, children.end());

    // connect internal node to the same parent as the node
    newInternal->parent = node->parent;

    // if we start at root, need to create a internal node
    if (node == root) {
        Node* newRoot = new Node(false);

        // set values for node
        newRoot->keys.push_back(separatorKey);
        newRoot->children.push_back(node);
        newRoot->children.push_back(newInternal);

        // set parents
        node->parent = newRoot;
        newInternal->parent = newRoot;

        root = newRoot;
    } else {
        insertIntoParent(node, separatorKey, newInternal);
    }
}

void BPlusTree::insertIntoParent(Node* left, int separatorKey, Node* right) {
    Node* parent = left -> parent;
    std::vector<int>& keys = parent->keys;
    std::vector <Node*>& children = parent -> children;

    // find left node
    size_t index = 0;
    while(children[index] != left) {
        index++;
    }

    // insert new key and a new child to hold additional range
    // node with n keys had n+1 children
    keys.insert(keys.begin() + index, separatorKey);
    children.insert(children.begin() + index + 1, right);

    right->parent = parent;

    if (parent->keys.size() > MAX_KEYS) {
        splitInternal(parent);
    }
}

void BPlusTree::clear() {
    deleteTree(root);
    root = new Node(true);
}