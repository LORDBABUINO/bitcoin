// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_UPDATEUTREEXO_H
#define BITCOIN_INDEX_UPDATEUTREEXO_H

#include <index/base.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <util/fs.h>

struct UtreexoForest;

namespace interfaces {
class Chain;
}

static constexpr bool DEFAULT_UPDATE_UTREEXO{false};

class UpdateUtreexo final : public BaseIndex
{
protected:
    class DB;

private:
    const std::unique_ptr<DB> m_db;
    UtreexoForest* m_forest;
    const fs::path m_utreexo_path;

    bool AllowPrune() const override { return false; }

    bool LoadForest();
    bool SaveForest();
    std::pair<std::vector<uint8_t>, size_t> CollectSpentHashes(
        const interfaces::BlockInfo& block, const unsigned char* utreexo_tag);
    bool ProcessBlock(const interfaces::BlockInfo& block,
                      std::vector<uint8_t>& add_hashes, size_t add_count,
                      std::vector<uint8_t>& del_hashes, size_t del_count);

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;
    bool CustomRemove(const interfaces::BlockInfo& block) override;
    BaseIndex::DB& GetDB() const override;
    interfaces::Chain::NotifyOptions CustomOptions() override;

public:
    explicit UpdateUtreexo(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size,
                           bool f_memory = false, bool f_wipe = false);
    virtual ~UpdateUtreexo() override;
};

/// The global Utreexo index object.
extern std::unique_ptr<UpdateUtreexo> g_updateutreexo;

#endif
