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

// Create a new Utreexo forest
UtreexoForest* utreexo_forest_new(void);

// Add hashes to the forest
// hashes: pointer to array of 32-byte hashes
// num_hashes: number of hashes in the array
// Returns: 0 on success, -1 on failure
int utreexo_forest_add(UtreexoForest* forest, const uint8_t* hashes, size_t num_hashes);

// Free the Utreexo forest
void utreexo_forest_free(UtreexoForest* forest);

#ifdef __cplusplus
}
#endif

#endif // BITCOIN_UTIL_RUSTREEXO_H
