#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// how we manage the table’s load factor
// grow the array when it becomes at least 75% full
#define TABLE_MAX_LOAD 0.75

// added for ch 20.1 challenge
static uint32_t hashDouble(double value) {
    union BitCast {
        double value;
        uint32_t ints[2];
    };

    union BitCast cast;
    cast.value = (value) + 1.0;
    return cast.ints[0] + cast.ints[1];
}

// added for ch 20.1 challenge
// distinct constant hash codes for singleton values true, false, and nil
// so we know when the code evaluate to these keys
// use prime numbers to help spread the keys out across the hash table's buckets
/* ex. hashValue(true), returns 3
 * VM takes that 3 and calculates index = 3 % capacity
 * VM goes to entries[index] and stores {true: "yes"}
 */
uint32_t hashValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:   return AS_BOOL(value) ? 3 : 5;
        case VAL_NIL:    return 7;
        case VAL_NUMBER: return hashDouble(AS_NUMBER(value));
        case VAL_OBJ:    return AS_STRING(value)->hash;
        case VAL_EMPTY:  return 0;
    }
}

// hash table starts with 0 capacity and NULL array
void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

// don’t need to check for NULL since FREE_ARRAY() handles that
void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

// // function responsible for taking a key and an array of buckets,
// // and figuring out which bucket the entry belongs in
// // findEntry() to look up existing entries in the hash table and to decide where to insert new entries
// static Entry* findEntry(Entry* entries, int capacity,
//                         ObjString* key) {
//     // modulo to map the key’s hash code to an index within the array’s bounds
//     uint32_t index = key->hash % capacity;
//
//     // the first time passing a tombstone, store it
//     Entry* tombstone = NULL;
//
//     // for loop probes for empty bucket to put key-value pairs into
//     // no infinite loop because of TABLE_MAX_LOAD, always have empty buckets
//     for (;;) {
//         Entry* entry = &entries[index];
//         // hit a tombstone then note it and keep going to check for key further down the array
//         if (entry->key == NULL) {
//             // if don't find key and need to store value
//             // store value in first tombstone spot instead of empty bucket later down the line
//             if (IS_NIL(entry->value)) {
//                 // empty entry
//                 return tombstone != NULL ? tombstone : entry;
//             } else {
//                 // found a tombstone
//                 if (tombstone == NULL) tombstone = entry;
//             }
//         } else if (entry->key == key) {
//             // found the key
//             return entry;
//         }
//         // modulo operator to wrap back around to the beginning of the array
//         // if we get to the end before placing the key-value
//         index = (index + 1) % capacity;
//     }
// }

// added for ch 20.1 challenge
static Entry* findEntry(Entry* entries, int capacity, Value key) {
    uint32_t index = hashValue(key) % capacity;
    Entry* tombstone = NULL;

    for (;;) {
        Entry* entry = &entries[index];
        if (IS_EMPTY(entry->key)) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (valuesEqual(key, entry->key)) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

// given a key, look up the corresponding value if it exists
// bool tableGet(Table* table, ObjString* key, Value* value) {
// added for ch 20.1 challenge
bool tableGet(Table* table, Value key, Value* value) {
    // no value is table is empty
    if (table->count == 0) return false;

    // no value if the key does not exist yet
    Entry* entry = findEntry(table->entries, table->capacity, key);
    // if (entry->key == NULL) return false;
    // added/changed for ch 20.1 challenge
    if (IS_EMPTY(entry->key)) return false;

    // return that there is a value if the key is in the hash table
    *value = entry->value;
    return true;
}

// allocate an array of buckets
static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        // entries[i].key = NULL;
        // added/changed for ch 20.1 challenge
        entries[i].key = EMPTY_VAL;
        entries[i].value = NIL_VAL;
    }

    // new array size means entries may end up in different buckets (new possibility of collision)
    // solve by re-inserting every entry into the new empty array
    table->count = 0; // don’t copy the tombstones over
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        // added/changed for ch 20.1 challenge
        if (IS_EMPTY(entry->key)) continue;
        // if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        // increment count when non-tombstone entry
        table->count++;
    }

    // release the memory for the old array
    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
    // may end up with fewer entries in the resulting larger array because all the tombstones get discarded
}

// adds the given key/value pair to the given hash table
// if an entry for that key is already present, the new value overwrites the old value
// the function returns true if a new entry was added
// bool tableSet(Table* table, ObjString* key, Value value) {
// added for ch 20.1 challenge
bool tableSet(Table* table, Value key, Value value) {
    // allocated the Entry array, making sure it is big enough
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    // bool isNewKey = entry->key == NULL;
    // added/changed for ch 20.1 challenge
    bool isNewKey = IS_EMPTY(entry->key);
    // only increase count if adding a new key, not when overwriting
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

// tombstone: when following a probe sequence during a lookup, hit a tombstone, don’t stop iterating
// instead, keep going through collision chain(s) and still find entries after it deleted entry
// bool tableDelete(Table* table, ObjString* key) {
// added for ch 20.1 challenge
bool tableDelete(Table* table, Value key) {
    // exit if nothing to delete
    if (table->count == 0) return false;

    // find the entry
    Entry* entry = findEntry(table->entries, table->capacity, key);
    // if (entry->key == NULL) return false;
    // added/changed for ch 20.1 challenge
    if (IS_EMPTY(entry->key)) return false;

    // place a tombstone (NULL: true) in the entry
    // entry->key = NULL;
    // added/changed for ch 20.1 challenge
    entry->key = EMPTY_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

// helper function for copying all the entries of one hash table into a new hash table
void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        // if (entry->key != NULL) {
        // added for ch 20.1 challenge
        if (!IS_EMPTY(entry->key)) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

// to look for a string in the string table
// precedes findEntry() because findEntry() would have duplicate keys without this function
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry* entry = &table->entries[index];
        // if (entry->key == NULL) {
        //     // Stop if we find an empty non-tombstone entry.
        //     if (IS_NIL(entry->value)) return NULL;
        // } else if (entry->key->length == length &&
        //     entry->key->hash == hash &&
        //     memcmp(entry->key->chars, chars, length) == 0) {
        //     // We found it.
        //     return entry->key;
        // }
        // added/changed for ch 20.1 challenge
        if (IS_EMPTY(entry->key)) {
            if (IS_NIL(entry->value)) return NULL;
        } else {
            ObjString* string = AS_STRING(entry->key); // Extract the string pointer
            if (string->length == length &&
                string->hash == hash &&
                memcmp(string->chars, chars, length) == 0) {
                return string;
                }
        }

        index = (index + 1) % table->capacity;
    }
}
