#pragma once
#include "../engine/cache.h"
#include <unordered_map>
#include <string>
#include <iostream>

class LRU : public Cache {
private:
    // The node structure for our Doubly Linked List
    struct Node {
        std::string key;
        Node* prev;
        Node* next;
        
        Node(std::string k) : key(k), prev(nullptr), next(nullptr) {}
    };

    // Pointers to the dummy head and tail
    Node* head;
    Node* tail;

    // The O(1) lookup table
    std::unordered_map<std::string, Node*> map;

    // --- Helper Functions (The "Surgery" Tools) ---
    
    // 1. Add a new node right after head (Most Recently Used)
    void add_node(Node* node) {
        Node* temp = head->next;
        
        node->next = temp;
        node->prev = head;
        
        head->next = node;
        temp->prev = node;
    }

    // 2. Remove an existing node from the list
    void delete_node(Node* node) {
        Node* prev_node = node->prev;
        Node* next_node = node->next;
        
        prev_node->next = next_node;
        next_node->prev = prev_node;
    }

    // 3. Move an existing node to the front
    void move_to_head(Node* node) {
        delete_node(node);
        add_node(node);
    }

public:
    explicit LRU(std::size_t cap) : Cache(cap) {
        // Initialize Sentinels
        head = new Node("");
        tail = new Node("");
        
        // Connect them: [Head] <-> [Tail]
        head->next = tail;
        tail->prev = head;
    }

    ~LRU() {
        // Cleanup memory to avoid leaks!
        Node* current = head;
        while (current != nullptr) {
            Node* temp = current;
            current = current->next;
            delete temp;
        }
    }

    bool access(std::string key) override {
        // 1. Check Hit
        if (map.find(key) != map.end()) {
            Node* node = map[key];
            move_to_head(node); // "Refreshed"
            hits++;
            return true;
        }

        // 2. Handle Miss
        misses++;
        
        if (map.size() >= capacity) {
            // Evict the LRU (The one before tail)
            Node* lru_node = tail->prev;
            
            // Remove from Map and List
            map.erase(lru_node->key);
            delete_node(lru_node);
            delete lru_node; // Free memory
        }

        // 3. Add new node
        Node* new_node = new Node(key);
        add_node(new_node);
        map[key] = new_node;
        
        return false;
    }
};
