// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/updateutreexo.h>

#include <common/args.h>
#include <logging.h>
#include <util/rustreexo.h>

std::unique_ptr<UpdateUtreexo> g_updateutreexo;

class UpdateUtreexo::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);
};

UpdateUtreexo::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "updateutreexo",
                    n_cache_size, f_memory, f_wipe)
{}

UpdateUtreexo::UpdateUtreexo(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size,
                             bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "updateutreexo"),
      m_db(std::make_unique<UpdateUtreexo::DB>(n_cache_size, f_memory, f_wipe))
{}

UpdateUtreexo::~UpdateUtreexo() = default;

bool UpdateUtreexo::CustomAppend(const interfaces::BlockInfo& block) {
    printHello();
    return true;
}

BaseIndex::DB& UpdateUtreexo::GetDB() const { return *m_db; }
