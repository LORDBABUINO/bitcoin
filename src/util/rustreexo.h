// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RUSTREEXO_H
#define BITCOIN_UTIL_RUSTREEXO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer to Rust UtreexoForest struct
typedef struct UtreexoForest UtreexoForest;

// Buffer struct for returning data from Rust
typedef struct {
    uint8_t* data;
    size_t len;
} Buffer;

// Create a new Utreexo forest
UtreexoForest* utreexo_forest_new(void);

// Modify the forest by adding and/or deleting hashes
// forest: pointer to the forest
// add_hashes: pointer to array of 32-byte hashes to add (can be NULL if num_add is 0)
// num_add: number of hashes to add
// del_hashes: pointer to array of 32-byte hashes to delete (can be NULL if num_del is 0)
// num_del: number of hashes to delete
// Returns: 0 on success, -1 on failure
int utreexo_forest_modify(UtreexoForest* forest, const uint8_t* add_hashes, size_t num_add,
                          const uint8_t* del_hashes, size_t num_del);

// Serialize the Utreexo forest to bytes
// forest: pointer to the forest
// Returns: Buffer with data pointer and length (caller must free with utreexo_free_buffer)
Buffer utreexo_forest_serialize(UtreexoForest* forest);

// Deserialize the Utreexo forest from bytes
// data: pointer to the serialized data
// len: length of the data
// Returns: pointer to the loaded forest, or NULL on failure
UtreexoForest* utreexo_forest_deserialize(const uint8_t* data, size_t len);

// Free a buffer allocated by Rust
// data: pointer to the buffer to free
void utreexo_free_buffer(uint8_t* data);

// Free the Utreexo forest
void utreexo_forest_free(UtreexoForest* forest);

#ifdef __cplusplus
}
#endif

#endif // BITCOIN_UTIL_RUSTREEXO_H
