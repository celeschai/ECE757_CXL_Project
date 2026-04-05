#include "ideal_prefetch.h"

#include <fmt/core.h>

#include "cache.h"
#include "oracle_prefetch_queue.h"

static constexpr std::size_t LOOKAHEAD = 16;

void ideal_prefetch::prefetcher_initialize()
{
  pf_issued = 0;
  pf_useful = 0;
}

uint32_t ideal_prefetch::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                                  uint32_t metadata_in)
{
  if (useful_prefetch)
    pf_useful++;

  auto& oracle = OraclePrefetchQueue::get();

  for (std::size_t n = 0; n < LOOKAHEAD && !oracle.empty(); ++n) {
    const uint64_t future_vaddr = oracle.queue().front();

    // Align to cache block boundary
    champsim::address pf_addr{champsim::block_number{champsim::address{future_vaddr}}};

    // Issue prefetch regardless of whether data is in DDR or CXL.
    // prefetch_line() routes the request down the hierarchy to whichever
    // memory tier currently holds the page - no tier filtering needed.
    bool success = prefetch_line(pf_addr, true, metadata_in);
    if (success)
      pf_issued++;

    // oracle.pop_front();
  }

  return metadata_in;
}

uint32_t ideal_prefetch::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr,
                                               uint32_t metadata_in)
{
  return metadata_in;
}

void ideal_prefetch::prefetcher_final_stats()
{
  fmt::print("=== Ideal Prefetcher Stats ===\n");
  fmt::print("  Prefetches issued : {}\n", pf_issued);
  fmt::print("  Useful prefetches : {}\n", pf_useful);
  if (pf_issued > 0)
    fmt::print("  Accuracy          : {:.1f}%\n", 100.0 * pf_useful / pf_issued);
}
