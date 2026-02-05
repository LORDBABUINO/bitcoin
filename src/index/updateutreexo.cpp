// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <common/args.h>
#include <crypto/sha2.hpp>
#include <crypto/sha512.h>
#include <hash.h>
#include <index/updateutreexo.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <undo.h>
#include <util/fs.h>
#include <util/readwritefile.h>
#include <util/rustreexo.h>

#include <cstring>
#include <future>
#include <stdexcept>
#include <vector>

// Compute Utreexo tag: SHA-512("UtreexoV1")
static const unsigned char* GetUtreexoTag()
{
    static unsigned char tag[CSHA512::OUTPUT_SIZE];
    static bool initialized = false;
    if (!initialized) {
        const char* tag_string = "UtreexoV1";
        CSHA512().Write(reinterpret_cast<const unsigned char*>(tag_string), strlen(tag_string)).Finalize(tag);
        initialized = true;
    }
    return tag;
}

static sha2::sha256_hash ComputeLeafHash(
    const unsigned char* utreexo_tag,
    const uint8_t* block_hash_data,
    const uint8_t* txid_data,
    uint32_t vout,
    uint32_t height,
    bool is_coinbase,
    const CTxOut& output)
{
    uint32_t header_code = (height << 1) | (is_coinbase ? 1 : 0);

    DataStream script_stream;
    script_stream << output.scriptPubKey;

    std::vector<uint8_t> preimage;
    preimage.reserve(64 + 64 + 32 + 32 + 4 + 4 + 8 + script_stream.size());

    preimage.insert(preimage.end(), utreexo_tag, utreexo_tag + 64);
    preimage.insert(preimage.end(), utreexo_tag, utreexo_tag + 64);

    preimage.insert(preimage.end(), block_hash_data, block_hash_data + 32);

    preimage.insert(preimage.end(), txid_data, txid_data + 32);

    uint32_t vout_le = htole32(vout);
    const uint8_t* vout_bytes = reinterpret_cast<const uint8_t*>(&vout_le);
    preimage.insert(preimage.end(), vout_bytes, vout_bytes + 4);

    uint32_t header_code_le = htole32(header_code);
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header_code_le);
    preimage.insert(preimage.end(), header_bytes, header_bytes + 4);

    uint64_t amount_le = htole64(static_cast<uint64_t>(output.nValue));
    const uint8_t* amount_bytes = reinterpret_cast<const uint8_t*>(&amount_le);
    preimage.insert(preimage.end(), amount_bytes, amount_bytes + 8);

    const uint8_t* script_bytes = reinterpret_cast<const uint8_t*>(script_stream.data());
    preimage.insert(preimage.end(), script_bytes, script_bytes + script_stream.size());

    return sha2::sha512_256(preimage.data(), preimage.size());
}

std::unique_ptr<UpdateUtreexo> g_updateutreexo;

class UpdateUtreexo::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
};

UpdateUtreexo::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "updateutreexo",
                    n_cache_size, f_memory, f_wipe)
{
}

bool UpdateUtreexo::LoadForest()
{
    if (!fs::exists(m_utreexo_path)) {
        return false;
    }

    auto [ok, data] = ReadBinaryFile(m_utreexo_path);
    if (!ok) {
        LogWarning("Utreexo: Failed to read from %s\n", fs::PathToString(m_utreexo_path));
        return false;
    }

    m_forest = utreexo_forest_deserialize(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    if (!m_forest) {
        LogWarning("Utreexo: Failed to deserialize forest from %s\n", fs::PathToString(m_utreexo_path));
        return false;
    }

    return true;
}

bool UpdateUtreexo::SaveForest()
{
    if (!m_forest) {
        return false;
    }

    Buffer buffer = utreexo_forest_serialize(m_forest);
    if (buffer.data == nullptr) {
        LogError("Utreexo: Failed to serialize forest\n");
        return false;
    }

    std::string data(reinterpret_cast<const char*>(buffer.data), buffer.len);
    utreexo_free_buffer(buffer.data);

    if (!WriteBinaryFile(m_utreexo_path, data)) {
        LogError("Utreexo: Failed to write to %s\n", fs::PathToString(m_utreexo_path));
        return false;
    }

    return true;
}

UpdateUtreexo::UpdateUtreexo(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size,
                             bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "updateutreexo"),
      m_db(std::make_unique<UpdateUtreexo::DB>(n_cache_size, f_memory, f_wipe)),
      m_forest(nullptr),
      m_utreexo_path(gArgs.IsArgSet("-utreexopath") ?
                         fs::PathFromString(gArgs.GetArg("-utreexopath", "")) :
                         gArgs.GetDataDirNet() / "utreexo" / "forest.dat")
{
    fs::create_directories(m_utreexo_path.parent_path());

    if (!LoadForest()) {
        m_forest = utreexo_forest_new();
        if (!m_forest) {
            throw std::runtime_error("Failed to create Utreexo forest");
        }
        LogInfo("Utreexo: Created new forest at %s\n", fs::PathToString(m_utreexo_path));
    } else {
        LogInfo("Utreexo: Loaded forest from %s\n", fs::PathToString(m_utreexo_path));
    }
}

UpdateUtreexo::~UpdateUtreexo()
{
    if (m_forest) {
        SaveForest();
        utreexo_forest_free(m_forest);
        m_forest = nullptr;
        LogInfo("Utreexo: Forest saved and freed\n");
    }
}

interfaces::Chain::NotifyOptions UpdateUtreexo::CustomOptions()
{
    interfaces::Chain::NotifyOptions options;
    options.connect_undo_data = true;
    return options;
}

bool UpdateUtreexo::CustomAppend(const interfaces::BlockInfo& block)
{
    // Skip genesis block
    if (block.height == 0) return true;

    if (!block.data) {
        LogWarning("UpdateUtreexo: Block data not available for height %d\n", block.height);
        return false;
    }

    const unsigned char* utreexo_tag = GetUtreexoTag();

    auto add_future = std::async(std::launch::async, [&]() {
        std::vector<uint8_t> hashes;
        size_t count = 0;

        for (const auto& tx : block.data->vtx) {
            const Txid& txid = tx->GetHash();
            const bool is_coinbase = tx->IsCoinBase();

            for (uint32_t vout = 0; vout < tx->vout.size(); ++vout) {
                sha2::sha256_hash leaf_hash = ComputeLeafHash(
                    utreexo_tag,
                    reinterpret_cast<const uint8_t*>(block.hash.data()),
                    reinterpret_cast<const uint8_t*>(txid.data()),
                    vout,
                    static_cast<uint32_t>(block.height),
                    is_coinbase,
                    tx->vout[vout]);

                hashes.insert(hashes.end(), leaf_hash.begin(), leaf_hash.end());
                ++count;
            }
        }

        return std::make_pair(std::move(hashes), count);
    });

    // Collect deletion hashes on the current thread
    std::vector<uint8_t> del_hashes;
    size_t total_del = 0;

    if (block.undo_data) {
        for (size_t i = 0; i < block.undo_data->vtxundo.size(); ++i) {
            const auto& tx = block.data->vtx[i + 1]; // +1 to skip coinbase
            const auto& txundo = block.undo_data->vtxundo[i];

            for (size_t j = 0; j < txundo.vprevout.size(); ++j) {
                const Coin& coin = txundo.vprevout[j];
                const COutPoint& prevout = tx->vin[j].prevout;

                uint256 creating_block_hash = m_chain->getBlockHash(coin.nHeight);

                sha2::sha256_hash leaf_hash = ComputeLeafHash(
                    utreexo_tag,
                    reinterpret_cast<const uint8_t*>(creating_block_hash.data()),
                    reinterpret_cast<const uint8_t*>(prevout.hash.data()),
                    prevout.n,
                    coin.nHeight,
                    coin.fCoinBase,
                    coin.out);

                del_hashes.insert(del_hashes.end(), leaf_hash.begin(), leaf_hash.end());
                ++total_del;
            }
        }
    }

    auto [add_hashes, total_add] = add_future.get();

    const uint8_t* add_ptr = add_hashes.empty() ? nullptr : add_hashes.data();
    const uint8_t* del_ptr = del_hashes.empty() ? nullptr : del_hashes.data();

    if (total_add > 0 || total_del > 0) {
        int result = utreexo_forest_modify(m_forest, add_ptr, total_add, del_ptr, total_del);
        if (result != 0) {
            LogError("UpdateUtreexo: Failed to modify forest at height %d (+%zu -%zu)\n",
                     block.height, total_add, total_del);
            return false;
        }
        LogDebug(BCLog::ALL, "UpdateUtreexo: Modified forest at height %d: +%zu -%zu UTXOs\n",
                 block.height, total_add, total_del);
    }

    if (!SaveForest()) {
        LogError("UpdateUtreexo: Failed to save forest after block %d\n", block.height);
        return false;
    }

    return true;
}

BaseIndex::DB& UpdateUtreexo::GetDB() const { return *m_db; }
