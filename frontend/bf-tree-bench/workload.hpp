/**
 * @file workload.hpp
 * @brief Functions to execute minimal example.
 *
 */

#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include "leanstore/utils/ZipfGenerator.hpp"

u64 scale;

static uint64_t total_record_cnt = 50'000'000;

static leanstore::utils::ZipfGenerator zipf_randgen(total_record_cnt, 0.9);

std::atomic<uint64_t> loaded_cnt;

// [0, n)
uint64_t rnd(uint64_t n)
{
   return leanstore::utils::RandomGenerator::getRand<uint64_t>(0, n);
}

uint64_t zipf_rand()
{
   return zipf_randgen.rand();
}

// murmur64
uint64_t getInt(uint64_t h)
{
   h ^= h >> 33;
   h *= 0xff51afd7ed558ccdULL;
   h ^= h >> 33;
   h *= 0xc4ceb9fe1a85ec53ULL;
   h ^= h >> 33;

   return h;
}

// -------------------------------------------------------------------------------------
// Functions to add data to dataset e.g. initialize dataset
// -------------------------------------------------------------------------------------

void loadScaled(uint64_t start, uint64_t end)
{
   for (auto i = start; i < end; i++) {
      scaled_table.insert({bf_record_t::make_key(getInt(i))}, {bf_record_t::make_record(getInt(i))});
      if (i % 500'000 == 0) {
         loaded_cnt += 500'000;
         cout << "Inserted " << loaded_cnt << " records" << endl;
      }
   }
}
// -------------------------------------------------------------------------------------
// Functions to execute operations on data
// -------------------------------------------------------------------------------------

bool checkOneScaled(uint64_t i)
{
   bool success = true;
   scaled_table.lookup1({bf_record_t::make_key(getInt(i))}, [&](const bf_record_t& record) { success = record.check_value(getInt(i)); });
   if (!success) {
      cout << "Scaled Failed: " << i << endl;
   }
   return success;
}

void fetchOneScaled()
{
   uint64_t id = zipf_rand();
   checkOneScaled(id);
}

void runOneQuery()
{
   fetchOneScaled();
}
