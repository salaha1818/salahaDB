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
    static constexpr size_t MIN_KEYS = 2;
    void clear();

    // deletion functions
    bool remove(int key);
    size_t findChildIndex(const Node* parent, const Node* child) const;
    size_t minLeafKeys() const;
    size_t minInternalKeys() const;
    bool hasMinimumKeys(const Node* node) const;
    void updateParentSeparator(Node* node);
    Node* getLeftSibling(Node* node) const;
    Node* getRightSibling(Node* node) const;
    bool canBorrowFrom(const Node* sibling) const;

    bool borrowFromLeftLeaf(Node* node, Node* leftSibling);
    bool borrowFromRightLeaf(Node* node, Node* rightSibling);
    bool borrowFromLeftInternal(Node* node, Node* leftSibling);
    bool borrowFromRightInternal(Node* node, Node* rightSibling);

    bool mergeLeafWithLeft(Node* node, Node* leftSibling);
    bool mergeLeafWithRight(Node* node, Node* rightSibling);
    bool mergeInternalWithLeft(Node* node, Node* leftSibling);
    bool mergeInternalWithRight(Node* node, Node* rightSibling);

    void rebalance(Node* node);

    void printTree() const;
    void printLevel(const std::vector<Node*>& level) const;

private:
    void deleteTree(Node* node);
};
