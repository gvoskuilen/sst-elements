// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_INST_LOADER
#define _H_VANADIS_INST_LOADER

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/subcomponent.h>

#include <cinttypes>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "vanadisDbgFlags.h"

#include "datastruct/vcache.h"
#include "vinsbundle.h"

namespace SST {
namespace Vanadis {

enum class VanadisInstructionLoaderMode {
    INFINITE_CACHE_MODE,
    LRU_CACHE_MODE
};

class VanadisInstructionLoader {
public:
    VanadisInstructionLoader() {}
    virtual ~VanadisInstructionLoader() = default;

    virtual uint64_t getCacheLineWidth() const = 0;
    virtual void setCacheLineWidth(uint64_t new_line_width) = 0;
    virtual void setMemoryInterface(SST::Interfaces::StandardMem* iface) = 0;

    virtual bool acceptResponse(SST::Interfaces::StandardMem::Request* req) = 0;
    virtual bool getPredecodeBytes(uint64_t addr, uint8_t* buffer, size_t buffer_req) = 0;
    virtual VanadisInstructionBundle* getBundleAt(const uint64_t addr) = 0;

    virtual void cacheDecodedBundle(VanadisInstructionBundle* bundle) = 0;
    virtual void clearCache() = 0;

    virtual bool hasBundleAt(uint64_t addr) const = 0;
    virtual bool hasPredecodeAt(uint64_t addr, uint64_t len) const = 0;

    virtual void requestLoadAt(uint64_t addr, uint64_t len) = 0;
    virtual void printStatus() = 0;
    virtual bool pendingLoad(uint64_t addr, uint64_t len) = 0;
};

template <SST::Vanadis::VanadisInstructionLoaderMode Mode>
class VanadisInstructionLoaderImpl : public VanadisInstructionLoader {
public:
    VanadisInstructionLoaderImpl(const size_t uop_cache_size, const size_t predecode_cache_entries,
                             const uint64_t cache_line_width, SST::Output* output) : VanadisInstructionLoader()
    {
        cache_line_width_ = cache_line_width;
        if constexpr (Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE) {
            uop_cache_ = new VanadisCache<uint64_t, VanadisInstructionBundle, SST::Vanadis::VanadisCacheRecordDeletion::VANADIS_PERFORM_DELETE>(uop_cache_size);
        }
        predecode_cache_ = new VanadisCache<uint64_t, uint8_t, SST::Vanadis::VanadisCacheRecordDeletion::VANADIS_PERFORM_DELETE_ARRAY>(predecode_cache_entries);
        output_ = output;

        mem_if_ = nullptr;
    }

    virtual ~VanadisInstructionLoaderImpl() {
        if constexpr (Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE) {
            delete uop_cache_;
        } else {
            for (auto& kv : uop_cache_) { delete kv.second; }
            uop_cache_.clear();
        }
        delete predecode_cache_;
    }

    uint64_t getCacheLineWidth() const override { return cache_line_width_; }

    void setCacheLineWidth(const uint64_t new_line_width) override
    {
        // if the line width is changed then we have to flush our cache
        if (cache_line_width_ != new_line_width) {
            predecode_cache_->clear();
        }

        cache_line_width_ = new_line_width;
    }

    void setMemoryInterface(SST::Interfaces::StandardMem* new_if) override { mem_if_ = new_if; }

    bool acceptResponse(SST::Interfaces::StandardMem::Request* req) override
    {
        // Looks like we created this request, so we should accept and process it
        auto pending_it = pending_loads_.find(req->getID());

        if (pending_it == pending_loads_.end()) {
            return false;
        }

        SST::Interfaces::StandardMem::ReadResp* resp = static_cast<SST::Interfaces::StandardMem::ReadResp*>(req);

        uint8_t* new_line = new uint8_t[cache_line_width_];
        std::memcpy(new_line, &resp->data[0], cache_line_width_);

        #ifdef VANADIS_BUILD_DEBUG
        const auto output_verbosity = output_->getVerboseLevel();
        if(output_verbosity >= 16) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[ins-loader] ---> response has: %" PRIu64 " bytes in payload\n",
                        (uint64_t)resp->data.size());

            char* print_line = new char[2048];
            char* temp_line = new char[2048];
            print_line[0] = '\0';

            for (uint64_t i = 0; i < cache_line_width_; ++i) {
                for (uint64_t j = 0; j < 2048; ++j) {
                    temp_line[j] = print_line[j];
                }

                snprintf(print_line, 2048, "%s %" PRIu8 "", temp_line, resp->data[i]);
            }

            if(output_verbosity >= 16) {
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[ins-loader] ---> cache-line: %s\n", print_line);
            }

            delete[] print_line;
            delete[] temp_line;
        }

        if(output_verbosity >= 16) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[ins-loader] ---> hit (addr=0x%" PRI_ADDR "), caching line in predecoder.\n",
                        resp->vAddr);
        }
        #endif
        predecode_cache_->store(resp->vAddr, new_line);

        // Remove from pending load stores.
        pending_loads_.erase(pending_it);

        return true;
    }

    bool getPredecodeBytes(const uint64_t addr, uint8_t* buffer, const size_t buffer_req) override
    {

        #ifdef VANADIS_BUILD_DEBUG
        // Very very unlikely given the code, only do this check during a debug build
        if (buffer_req > cache_line_width_) {
            output_->fatal(CALL_INFO, -1,
                          "[inst-predecode-bytes-check]: Requested a decoded bytes "
                          "fill of longer than a cache line, req=%" PRIu64 ", line-width=%" PRIu64 "\n",
                          (uint64_t)buffer_req, cache_line_width_);
        }
        const auto output_verbosity = output_->getVerboseLevel();
        #endif

        // calculate offset within the cache line
        const uint64_t inst_line_offset = (addr % cache_line_width_);

        // get the start of the cache line containing the request
        const uint64_t cache_line_start = addr - inst_line_offset;

        #ifdef VANADIS_BUILD_DEBUG
        if(output_verbosity >= 16) {
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG,
                        "[fill-decode]: ins-addr: 0x%" PRI_ADDR " line-offset: %" PRIu64 " line-start=%" PRIu64 " / 0x%" PRI_ADDR "\n",
                        addr, inst_line_offset, cache_line_start, cache_line_start);
        }
        #endif

        uint8_t* cached_bytes = predecode_cache_->find(cache_line_start);

        // Cache miss
        if ( cached_bytes == nullptr ) {
            #ifdef VANADIS_BUILD_DEBUG
            if(output_verbosity >= 16) {
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: line-filled: no\n");
            }
            #endif
            return false;
        }

        uint64_t bytes_from_this_line = std::min( static_cast<uint64_t>(buffer_req), cache_line_width_ - inst_line_offset );

        #ifdef VANADIS_BUILD_DEBUG
        if(output_verbosity >= 16) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: load %" PRIu64 " bytes from this line.\n", bytes_from_this_line);
        }
        #endif

        std::memcpy(buffer, &cached_bytes[inst_line_offset], bytes_from_this_line);

        // Cache hit and not split load (common case)
        if ( bytes_from_this_line == buffer_req ) {
            #ifdef VANADIS_BUILD_DEBUG
            if(output_verbosity >= 16) {
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: line-filled: yes\n");
            }
            #endif
            return true;
        }

        // Hit but split load
        #ifdef VANADIS_BUILD_DEBUG
        if(output_verbosity >= 16) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: requires split cache line load, first-line: %" PRIu64 " bytes\n", bytes_from_this_line);
        }
        #endif

        // Attempt to access second part of request
        cached_bytes = predecode_cache_->find(cache_line_start + cache_line_width_);

        if ( cached_bytes == nullptr ) {
            #ifdef VANADIS_BUILD_DEBUG
            if(output_verbosity >= 16) {
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: second line fill fails, line is not in predecode cache\n");
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: line-filled: no\n");
            }
            #endif
            return false;
        }

        std::memcpy(&buffer[bytes_from_this_line], &cached_bytes[0], (buffer_req - bytes_from_this_line));

        // Cache hit on second part of request too
        #ifdef VANADIS_BUILD_DEBUG
        if(output_verbosity >= 16) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: requires split cache line load, second-line: %" PRIu64 " bytes\n", (buffer_req - bytes_from_this_line));
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "[fill-decode]: line-filled: yes\n");
        }
        #endif


        return true;
    }

    void cacheDecodedBundle(VanadisInstructionBundle* bundle) override
    {
        uint64_t addr = bundle->getInstructionAddress();
        if constexpr (Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE) {
            uop_cache_->store(addr, bundle);
        } else {
            auto it = uop_cache_.find(addr);
            if (it != uop_cache_.end()) {
                delete it->second;
                it->second = bundle;
            } else {
                uop_cache_.emplace(addr, bundle);
            }
        }
    }

    void clearCache() override
    {
        if constexpr (Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE) {
            uop_cache_->clear();
        } else {
            for (auto& kv : uop_cache_) { delete kv.second; }
            uop_cache_.clear();
        }
        predecode_cache_->clear();
    }

    bool hasBundleAt(const uint64_t addr) const override
    {
        if constexpr (Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE) {
            return uop_cache_->contains(addr);
        } else {
            return !(uop_cache_.find(addr) == uop_cache_.end());
        }
    }

	bool hasPredecodeAt(const uint64_t addr, const uint64_t len) const override
    {
		const uint64_t line_start    = addr - (addr % cache_line_width_);
		const uint64_t len_line_left = cache_line_width_ - (addr % cache_line_width_);

		if (len <= len_line_left) {
			return predecode_cache_->contains(line_start);
		} else {
			const uint64_t line_start_right = line_start + cache_line_width_;
			return predecode_cache_->contains(line_start) && predecode_cache_->contains(line_start_right);
		}
	}

    VanadisInstructionBundle* getBundleAt(const uint64_t addr) override
    {
        if constexpr (Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE) {
            return uop_cache_->find(addr);
        } else {
            auto it = uop_cache_.find(addr);

            return (it == uop_cache_.end()) ? nullptr : it->second;
        }
    }

    void requestLoadAt(const uint64_t addr, const uint64_t len) override
    {

        #ifdef VANADIS_BUILD_DEBUG
        // Structurally impossible unless we're modifying this code so skip except in debug
        if (len > cache_line_width_) {
            output_->fatal(CALL_INFO, -1,
                          "Error: requested an instruction load which is longer than "
                          "a cache line, req=%" PRIu64 ", line=%" PRIu64 "\n",
                          len, cache_line_width_);
        }
        output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG,
                        "[ins-loader] ---> processing a requested load ip=%p, "
                        "icache-line: %" PRIu64 " bytes.\n",
                        (void*)addr, cache_line_width_);
        #endif

        const uint64_t line_start_offset = (addr % cache_line_width_);
        uint64_t line_start = addr - line_start_offset;

        do {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG,
                            "[ins-loader] ---> issue ins-load line-start: 0x%" PRI_ADDR ", line-len: %" PRIu64
                            " read-len=%" PRIu64 " \n",
                            line_start, line_start_offset, cache_line_width_);
            #endif

            bool cache_hit = predecode_cache_->touch(line_start);
            if ( !cache_hit ) {
	            bool found_pending_load = false;

	            for (const auto& pending_load_itr : pending_loads_) {
	                if (pending_load_itr.second->vAddr == line_start) {
	                    found_pending_load = true;
	                    break;
	                }
	            }

	            if (!found_pending_load) {
                    #ifdef VANADIS_BUILD_DEBUG
                    output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "[ins-loader] ----> creating a load for line at 0x%" PRI_ADDR ", len=%" PRIu64 "\n",
                        line_start, cache_line_width_);
                    #endif

	                SST::Interfaces::StandardMem::Read* req_line = new SST::Interfaces::StandardMem::Read(
                        line_start, cache_line_width_, 0, line_start);

	                pending_loads_.insert(
	                    std::pair<SST::Interfaces::StandardMem::Request::id_t, SST::Interfaces::StandardMem::Read*>(
	                        req_line->getID(), req_line));

	                mem_if_->send(req_line);

                #ifdef VANADIS_BUILD_DEBUG
	            } else {
   	             output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG,
      	                          "[ins-loader] -----> load is already in progress, will "
         	                       "not issue another load.\n");
                #endif
            	}
			}

            line_start += cache_line_width_;
        } while (line_start < (addr + len));

        #ifdef VANADIS_BUILD_DEBUG
		printPendingLoads();
        #endif
    }

    void printStatus() override
    {
        output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "Instruction Loader - Internal State Report:\n");
        output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "--> Cache Line Width:          %" PRIu64 "\n", cache_line_width_);
        output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "--> Pending Loads:             %" PRIu32 "\n",
                        (uint32_t)pending_loads_.size());

        for (auto pl_itr : pending_loads_) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_INS_LDR_FLG, "-----> Address:       %p\n", (void*)pl_itr.second->vAddr);
        }

        if constexpr (VanadisInstructionLoaderMode::INFINITE_CACHE_MODE == Mode) {
            output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "--> uop Cache Entries:         %zu / inf\n",
                            uop_cache_.size());
        } else {
            output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "--> uop Cache Entries:         %zu / %zu\n",
                            uop_cache_->size(), uop_cache_->capacity());
        }
        output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "--> Predecode Cache Entries:   %zu / %zu\n",
                        predecode_cache_->size(), predecode_cache_->capacity());
    }

    bool pendingLoad(const uint64_t addr, const uint64_t len) override
    {
        const uint64_t line_start_offset = (addr % cache_line_width_);
        const uint64_t line_start = addr - line_start_offset;
        for (const auto& pending_load_itr : pending_loads_) {
	        if (pending_load_itr.second->vAddr == line_start) {
	            return true;
	        }
	    }
        return false;
    }

private:

    // All calls are ifdef'd by VANADIS_BUILD_DEBUG
	void printPendingLoads() {
		output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "[ins-loader]: Pending loads table\n");
		for( const auto& next_load : pending_loads_ ) {
			output_->verbose(CALL_INFO, 8, VANADIS_DBG_INS_LDR_FLG, "[ins-loader]:   load: 0x%" PRI_ADDR " / size %" PRIu64 "\n",
				next_load.second->vAddr, next_load.second->size);
		}
	}

    uint64_t cache_line_width_;
    SST::Interfaces::StandardMem* mem_if_;

    VanadisCache<uint64_t, uint8_t, SST::Vanadis::VanadisCacheRecordDeletion::VANADIS_PERFORM_DELETE_ARRAY>* predecode_cache_;
    std::unordered_map<SST::Interfaces::StandardMem::Request::id_t, SST::Interfaces::StandardMem::Read*> pending_loads_;

    using BundleCacheType = std::conditional_t<Mode == VanadisInstructionLoaderMode::LRU_CACHE_MODE,
        VanadisCache<uint64_t, VanadisInstructionBundle, SST::Vanadis::VanadisCacheRecordDeletion::VANADIS_PERFORM_DELETE>*,
        std::unordered_map<uint64_t, VanadisInstructionBundle*>>;

    BundleCacheType uop_cache_;

    SST::Output* output_;
};

} // namespace Vanadis
} // namespace SST

#endif
