#pragma once
#define BPLUSTREE_H

#include <vector>
#include <optional>
#include "PageManager.h"
#include "RecordLocation.h"

class BPlusTree {
private:
    // leaf node uses keys, values, next leaf and parent, holds the actual index entries
    // internal node uses keys, children, and parent, gives directions to a speciifc child based on range
    struct Node {
        bool isLeaf;
        std::vector<int> keys;
        std::vector<RecordLocation> values;

        Node* parent;
        Node* nextLeaf;

        std::vector<Node*> children;

        // Creates either a leaf node or an internal node
        // leaf - true, values is used
        // internal - false, children is used 
        Node(bool leaf) {
            isLeaf = leaf;
            parent = nullptr;
            nextLeaf = nullptr;
        }
    };

    Node* root;

public:
    BPlusTree() {
        root = new Node(true);
    }
    ~BPlusTree() {
        deleteTree(root);
    }

    void insert(int key, const RecordLocation& location);
    // optional so if not found, it will return null
    std::optional<RecordLocation> search(int key) const;
    Node* findLeaf(int key) const;
    void splitLeaf(Node* leaf);
    void insertIntoParent(Node* left, int separatorKey, Node* right);
    void splitInternal(Node* node);
    static constexpr size_t MAX_KEYS = 4;
    void clear();

private:
    void deleteTree(Node* node);
};
