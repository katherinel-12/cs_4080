#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "common.h"
#include "value.h"

// key is always a string, value can be anything
typedef struct {
    // ObjString* key;
    Value key; // added/changed for ch 20.1 challenge
    Value value;
} Entry;

// building a Hash Table: an array of entries
// keep track of:
//   allocated size of the array (capacity)
//   the number of key/value pairs currently stored in it (count) (and tombstones for deleted entries)
// ratio of count to capacity is the load factor of the hash table
// load factor: collision likelihood, higher load factor means greater chance of collisions
typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

// to create a new, empty hash table
void initTable(Table* table);

// to free the table memory
void freeTable(Table* table);

// given a key, look up the corresponding value if it exists
// bool tableGet(Table* table, ObjString* key, Value* value);
// added for ch 20.1 challenge
bool tableGet(Table* table, Value key, Value* value);

// deleting from a hash table
// bool tableDelete(Table* table, ObjString* key);
// added for ch 20.1 challenge
bool tableDelete(Table* table, Value key);

// here, string objects know their hash code, so put them into hash tables
// bool tableSet(Table* table, ObjString* key, Value value);
// added for ch 20.1 challenge
bool tableSet(Table* table, Value key, Value value);

// helper function for copying all the entries of one hash table into a new hash table
void tableAddAll(Table* from, Table* to);

// to look for a string in the string table
// precedes findEntry() because findEntry() would have duplicate keys without this function
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash);

#endif //CLOX_TABLE_H