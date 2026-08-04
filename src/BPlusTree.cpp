#define BPLUSTREE_H

#include <vector>
#include <optional>
#include "PageManager.h"
#include "Database.h"
#include "BPlusTree.h"
#include <algorithm>
#include <stdexcept>
#include <queue>
#include <iostream>



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

bool BPlusTree::remove(int key) {
    Node* leaf = findLeaf(key);

    // find index of key 
    auto it = std::find(leaf->keys.begin(), leaf->keys.end(), key);

    // if invalid return
    if (it == leaf->keys.end() || *it != key) {
        return false;
    }

    size_t index = static_cast<size_t>(
        it - leaf->keys.begin()
    );

    // erase old keys and values
    leaf->keys.erase(leaf->keys.begin() + index);
    leaf->values.erase(leaf->values.begin() + index);

    if (index == 0 && !leaf->keys.empty()) {
        updateParentSeparator(leaf);
    }
    rebalance(leaf);
    return true;
}

size_t BPlusTree::findChildIndex(const Node* parent, const Node* child) const {
    for(size_t i = 0; i < parent -> children.size(); i++){
        if(parent -> children[i] == child) {
            return i; 
        }
    }
    throw std::runtime_error("Child was not found in its parent.");
}

// seperate incase ! change it later
size_t BPlusTree::minLeafKeys() const {
    return MIN_KEYS;
}
size_t BPlusTree::minInternalKeys() const {
    return MIN_KEYS;
}

bool BPlusTree::hasMinimumKeys(const Node* node) const {
    if (node == root) {
        return true;
    } else if (node->isLeaf) {
        return node->keys.size() >= minLeafKeys();
    } else {
        return node->keys.size() >= minInternalKeys();
    }
}

void BPlusTree::updateParentSeparator(Node* node) {
    if(node == root) return;
    if (node->keys.empty()) return;

    Node* parent = node -> parent;
    size_t index = findChildIndex(parent, node);

    if(index == 0) return;
    size_t prevIndex = index-1;
    parent -> keys[prevIndex] = node->keys[0];
}

BPlusTree::Node* BPlusTree::getLeftSibling(Node* node) const {
    if (node == root || node->parent == nullptr) {
        return nullptr;
    }

    Node* parent = node->parent;
    size_t index = findChildIndex(parent, node);

    if (index == 0) {
        return nullptr;
    }

    return parent->children[index - 1];
}

BPlusTree::Node* BPlusTree::getRightSibling(Node* node) const {
    if (node == root || node->parent == nullptr) {
        return nullptr;
    }

    Node* parent = node->parent;
    size_t index = findChildIndex(parent, node);

    if (index + 1 >= parent->children.size()) {
        return nullptr;
    }

    return parent->children[index + 1];
}

bool BPlusTree::canBorrowFrom(const Node* sibling) const {
    if (sibling == nullptr) return false; 
    if (sibling->isLeaf) return sibling->keys.size() > minLeafKeys();
    return sibling->keys.size() > minInternalKeys();
}

bool BPlusTree::borrowFromLeftLeaf(Node* node, Node* leftSibling) {
    if(canBorrowFrom(leftSibling)){
        RecordLocation location = leftSibling->values.back();
        int key = leftSibling->keys.back();

        node->keys.insert(node->keys.begin(), key);
        node->values.insert(node->values.begin(), location);

        leftSibling->keys.pop_back();
        leftSibling->values.pop_back();

        updateParentSeparator(node);
        return true;
    } 
    return false;
}

bool BPlusTree::borrowFromRightLeaf(Node* node, Node* rightSibling){
    if(canBorrowFrom(rightSibling)){
        RecordLocation location = rightSibling->values.front();
        int key = rightSibling->keys.front();

        node->keys.insert(node->keys.end(), key);
        node->values.insert(node->values.end(), location);

        rightSibling->keys.erase(rightSibling->keys.begin());
        rightSibling->values.erase(rightSibling->values.begin());

        updateParentSeparator(rightSibling);
        return true;
    } 
    return false;
}

bool BPlusTree::borrowFromLeftInternal(Node* node, Node* leftSibling){
    if(canBorrowFrom(leftSibling)){
        Node* parent = node -> parent;
        size_t index = findChildIndex(parent, node);

        // parent seperator goes to front of node keys
        int seperator = parent ->keys[index-1];
        node->keys.insert(node->keys.begin(), seperator);

        // left sibling last child goes to front of node keys
        Node* movedChild = leftSibling->children.back();
        node->children.insert(node->children.begin(), movedChild);
        movedChild -> parent = node;

        // left sibling last key becomes parent seperator
        parent->keys[index - 1] = leftSibling->keys.back();

        // remove the borrowed key and child
        leftSibling->keys.pop_back();
        leftSibling->children.pop_back();
        return true;
    }
    return false;
}

bool BPlusTree::borrowFromRightInternal(Node* node, Node* rightSibling) {
    if(canBorrowFrom(rightSibling)){
        Node* parent = node -> parent;
        size_t index = findChildIndex(parent, node);

        // parent seperator goes to end of node keys
        int seperator = parent ->keys[index];
        node->keys.push_back(seperator);

        // right sibling first child goes to end of node children
        Node* movedChild = rightSibling->children.front();
        node->children.push_back(movedChild);
        movedChild -> parent = node;

        // right sibling first key becomes parent seperator
        parent->keys[index] = rightSibling->keys.front();

        // remove the borrowed key and child
        rightSibling->keys.erase(rightSibling->keys.begin());
        rightSibling->children.erase(rightSibling->children.begin());
        return true;
    }
    return false;
}

bool BPlusTree::mergeLeafWithLeft(Node* node, Node* leftSibling) {
    Node* parent = node->parent;
    size_t index = findChildIndex(parent, node);
    
    // move all keys and values to left sibling
    leftSibling->keys.reserve(leftSibling->keys.size() + node->keys.size()); 
    leftSibling->keys.insert(leftSibling->keys.end(), node->keys.begin(), node->keys.end());
    leftSibling->values.reserve(leftSibling->values.size() + node->values.size()); 
    leftSibling->values.insert(leftSibling->values.end(), node->values.begin(), node->values.end());

    // remove node from the leaf linked list
    leftSibling->nextLeaf = node->nextLeaf;

    // remove the separator before the node
    parent->keys.erase(parent->keys.begin() + (index - 1));

    // remove the node's child pointer from the parent
    parent->children.erase(parent->children.begin() + index);
    
    delete node;
    return true;
}

bool BPlusTree::mergeLeafWithRight(Node* node, Node* rightSibling) {
    Node* parent = node->parent;
    size_t index = findChildIndex(parent, node);
    
    // move all keys and values to node 
    node->keys.reserve(node->keys.size() + rightSibling->keys.size()); 
    node->keys.insert(node->keys.end(), rightSibling->keys.begin(), rightSibling->keys.end());
    node->values.reserve(node->values.size() + rightSibling->values.size()); 
    node->values.insert(node->values.end(), rightSibling->values.begin(), rightSibling->values.end());

    // remove right sibling from the leaf linked list
    node->nextLeaf = rightSibling->nextLeaf;

    // remove the seperator at index
    parent->keys.erase(parent->keys.begin() + index);

    // remove the node's child pointer from the parent
    parent->children.erase(parent->children.begin() + index+1);
    
    delete rightSibling;
    return true;
}

bool BPlusTree::mergeInternalWithLeft(Node* node, Node* leftSibling) {
    Node* parent = node->parent;
    size_t index = findChildIndex(parent, node);

    // move the parent separator down into the end of leftSibling keys
    leftSibling->keys.reserve(leftSibling->keys.size() + 1 + node->keys.size());
    leftSibling->keys.push_back(parent->keys[index-1]);
    
    // move all keys and children to left sibling
    leftSibling->keys.insert(leftSibling->keys.end(), node->keys.begin(), node->keys.end());
    leftSibling->children.reserve(leftSibling->children.size() + node->children.size()); 
    leftSibling->children.insert(leftSibling->children.end(), node->children.begin(), node->children.end());

    // update parent pointer for moved children
    for (Node* child : node->children) {
        child->parent = leftSibling;
    }

    // remove separator from parents
    parent->keys.erase(parent->keys.begin() + index - 1);

    // remove the node's child pointer from the parent
    parent->children.erase(parent->children.begin() + index);

    delete node;
    return true;
}

bool BPlusTree::mergeInternalWithRight(Node* node, Node* rightSibling) {
    Node* parent = node->parent;
    size_t index = findChildIndex(parent, node);

    // move the parent separator down into the end of node keys
    node->keys.reserve(node->keys.size() + 1 + rightSibling->keys.size());
    node->keys.push_back(parent->keys[index]);
    
    // move all keys and children to node
    node->keys.insert(node->keys.end(), rightSibling->keys.begin(), rightSibling->keys.end());
    node->children.reserve(node->children.size() + rightSibling->children.size()); 
    node->children.insert(node->children.end(), rightSibling->children.begin(), rightSibling->children.end());

    // update parent pointer for moved children
    for (Node* child : rightSibling->children) {
        child->parent = node;
    }

    // remove separator between node and right sibling
    parent->keys.erase(parent->keys.begin() + index);

    // remove the right sibling's pointer from the parent
    parent->children.erase(parent->children.begin() + index + 1);

    delete rightSibling;
    return true;
}

void BPlusTree::rebalance(Node* node) {
    // root case
    if (node == root) {
        if (!node->isLeaf &&
            node->children.size() == 1 &&
            node->keys.empty()) {

            Node* oldRoot = root;
            root = oldRoot->children[0];
            root->parent = nullptr;
            delete oldRoot;
        }

        return;
    }

    // node is valid
    if(hasMinimumKeys(node)) return;

    Node* leftSibling = getLeftSibling(node);
    Node* rightSibling = getRightSibling(node);

    if(node->isLeaf) {
        if(borrowFromLeftLeaf(node, leftSibling)) {
            return;
        }
        if (borrowFromRightLeaf(node, rightSibling)) {
            return;
        }
    } else {
        if(borrowFromLeftInternal(node, leftSibling)) {
            return;
        }
        if (borrowFromRightInternal(node, rightSibling)) {
            return;
        }
    }

    // save parent because merging may delete node
    Node* parent = node->parent;
    bool merged = false;

    // nobody could lend, so merge
    if (node->isLeaf) {
        if (leftSibling != nullptr) {
            merged =mergeLeafWithLeft(node, leftSibling);
        } else if (rightSibling != nullptr) {
            merged =mergeLeafWithRight(node, rightSibling);
        }
    } else {
        if (leftSibling != nullptr) {
            merged =mergeInternalWithLeft(node, leftSibling);
        } else if (rightSibling != nullptr) {
            merged =mergeInternalWithRight(node, rightSibling);
        }
    }

    // the parent lost a key, so it may now underflow
    if (merged) {
        rebalance(parent);
    }
}

void BPlusTree::printTree() const {
    if (root == nullptr) {
        std::cout << "Tree is empty.\n";
        return;
    }

    std::queue<Node*> nodes;
    nodes.push(root);

    while (!nodes.empty()) {
        size_t levelSize = nodes.size();

        for (size_t i = 0; i < levelSize; i++) {
            Node* current = nodes.front();
            nodes.pop();

            std::cout << "[";

            for (size_t j = 0; j < current->keys.size(); j++) {
                std::cout << current->keys[j];

                if (j + 1 < current->keys.size()) {
                    std::cout << " ";
                }
            }

            std::cout << "] ";

            if (!current->isLeaf) {
                for (Node* child : current->children) {
                    nodes.push(child);
                }
            }
        }

        std::cout << '\n';
    }
}

std::vector<RecordLocation> BPlusTree::rangeSearch(int minKey, int maxKey) const {
    std::vector <RecordLocation> result;
    if(minKey>maxKey) return result;

    Node* currentLeaf = findLeaf(minKey);

    while(currentLeaf != nullptr){
        for(size_t i = 0; i < currentLeaf->keys.size(); i++) {
            int key = currentLeaf->keys[i];

            if (key > maxKey) {
                return result;
            }

            if (key >= minKey) {
                result.push_back(currentLeaf->values[i]);
            }
        }
        currentLeaf = currentLeaf->nextLeaf;
    }
    return result;
}