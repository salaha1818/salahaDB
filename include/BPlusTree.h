#pragma once
#define BPLUSTREE_H

#include <vector>
#include <optional>
#include "PageManager.h"
#include "RecordLocation.h"

// stores record locations using the record id as the key
class BPlusTree {
private:
    // leaf nodes store the actual keys and record locations
    // internal nodes store keys that direct searches to their children
    struct Node {
        bool isLeaf;

        std::vector<int> keys;

        // only used by leaf nodes
        std::vector<RecordLocation> values;

        Node* parent;

        // connects leaf nodes for range searches
        Node* nextLeaf;

        // only used by internal nodes
        std::vector<Node*> children;

        // creates either a leaf node or an internal node
        Node(bool leaf) {
            isLeaf = leaf;
            parent = nullptr;
            nextLeaf = nullptr;
        }
    };

    Node* root;

public:
    // starts the tree with one empty leaf
    BPlusTree() {
        root = new Node(true);
    }

    // deletes every node in the tree
    ~BPlusTree() {
        deleteTree(root);
    }

    // inserts a key and its record location
    void insert(
        int key,
        const RecordLocation& location
    );

    // returns the record location if the key exists
    std::optional<RecordLocation> search(int key) const;

    // follows internal nodes until the correct leaf is found
    Node* findLeaf(int key) const;

    // splits a leaf that has too many keys
    void splitLeaf(Node* leaf);

    // inserts a new separator key and child into the parent
    void insertIntoParent(
        Node* left,
        int separatorKey,
        Node* right
    );

    // splits an internal node that has too many keys
    void splitInternal(Node* node);

    // maximum and minimum keys allowed in each node
    static constexpr size_t MAX_KEYS = 4;
    static constexpr size_t MIN_KEYS = 2;

    // deletes the current tree and creates a new empty root
    void clear();

    // removes a key and rebalances the tree if needed
    bool remove(int key);

    // finds where a child is stored inside its parent
    size_t findChildIndex(
        const Node* parent,
        const Node* child
    ) const;

    size_t minLeafKeys() const;
    size_t minInternalKeys() const;

    // checks if a node still has enough keys
    bool hasMinimumKeys(const Node* node) const;

    // updates the parent key when the first key in a child changes
    void updateParentSeparator(Node* node);

    // returns the neighbouring nodes under the same parent
    Node* getLeftSibling(Node* node) const;
    Node* getRightSibling(Node* node) const;

    // checks if a sibling has an extra key it can lend
    bool canBorrowFrom(const Node* sibling) const;

    // borrow functions used when a node falls below the minimum
    bool borrowFromLeftLeaf(
        Node* node,
        Node* leftSibling
    );

    bool borrowFromRightLeaf(
        Node* node,
        Node* rightSibling
    );

    bool borrowFromLeftInternal(
        Node* node,
        Node* leftSibling
    );

    bool borrowFromRightInternal(
        Node* node,
        Node* rightSibling
    );

    // merge functions used when neither sibling can lend
    bool mergeLeafWithLeft(
        Node* node,
        Node* leftSibling
    );

    bool mergeLeafWithRight(
        Node* node,
        Node* rightSibling
    );

    bool mergeInternalWithLeft(
        Node* node,
        Node* leftSibling
    );

    bool mergeInternalWithRight(
        Node* node,
        Node* rightSibling
    );

    // borrows or merges until the node is valid again
    void rebalance(Node* node);

    // returns record locations with keys inside the given range
    std::vector<RecordLocation> rangeSearch(
        int minKey,
        int maxKey
    ) const;

private:
    // recursively deletes every node below this node
    void deleteTree(Node* node);
};