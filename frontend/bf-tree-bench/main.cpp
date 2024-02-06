
/**
 * @file main.cpp
 * @brief Manages the execution of the workload.
 *
 */

#include "../shared/LeanStoreAdapter.hpp"
#include "../shared/Types.hpp"
#include "leanstore/concurrency-recovery/CRMG.hpp"
#include "leanstore/profiling/counters/CPUCounters.hpp"
#include "leanstore/profiling/counters/WorkerCounters.hpp"
#include "leanstore/utils/Misc.hpp"
#include "leanstore/utils/Parallelize.hpp"
#include "leanstore/utils/RandomGenerator.hpp"
#include "schema.hpp"
// -------------------------------------------------------------------------------------
#include <gflags/gflags.h>

#include "PerfEvent.hpp"
// -------------------------------------------------------------------------------------
#include <unistd.h>

#include <iostream>
#include <set>
#include <string>
#include <vector>
// -------------------------------------------------------------------------------------
DEFINE_int64(scale, 1, "Should the scaling be increased, ca 100 mb per scale");
// -------------------------------------------------------------------------------------
using namespace std;
using namespace leanstore;
// -------------------------------------------------------------------------------------
LeanStoreAdapter<bf_record_t> scaled_table;
string db_meta;
bool load_db;
// -------------------------------------------------------------------------------------
#include "workload.hpp"
// -------------------------------------------------------------------------------------
void setup(LeanStore& db);
void loadDB(LeanStore& db);
void printLoadStatistics(LeanStore& db);
void run(LeanStore& db);
void printRunStatistics(LeanStore& db);

int main(int argc, char** argv)
{
   gflags::SetUsageMessage("Leanstore Minimal Example");
   gflags::ParseCommandLineFlags(&argc, &argv, true);
   assert(FLAGS_scale > 0);
   LeanStore::addS64Flag("min_SCALE", &FLAGS_scale);
   {
      LeanStore db;
      setup(db);
      loadDB(db);
      run(db);
   }
   return 0;
}

void setup(LeanStore& db)
{
   auto& crm = db.getCRManager();
   scale = FLAGS_scale;

   crm.scheduleJobSync(0, [&]() { scaled_table = LeanStoreAdapter<bf_record_t>(db, "scaled"); });
   db.registerConfigEntry("scale", FLAGS_scale);
}

void loadSimpleData();
void loadComplexData(std::atomic<u32>& g_w_id);

void loadDB(LeanStore& db)
{
   if (!FLAGS_recover) {
      auto& crm = db.getCRManager();
      std::atomic<u32> global_scale_factor(1);
      crm.scheduleJobs(FLAGS_worker_threads, [&]() { loadComplexData(global_scale_factor); });
      crm.joinAll();
   }
   printLoadStatistics(db);
}

void loadComplexData(std::atomic<u32>& global_scale_factor)
{
   auto worker_cnt = FLAGS_worker_threads;
   auto record_per_thread = total_record_cnt / worker_cnt;

   while (true) {
      u32 scale_fragment = global_scale_factor++;
      if (scale_fragment > worker_cnt) {
         return;
      }
      cr::Worker::my().startTX();
      loadScaled(record_per_thread * (scale_fragment - 1), record_per_thread * scale_fragment);
      cr::Worker::my().commitTX();
      if (cr::Worker::my().worker_id == 0) {
         cout << "Creation of scale " << scale_fragment << " of " << scale << " done." << endl;
      }
   }
}

template <class T>
void printer(string name, LeanStoreAdapter<T>& adapter, LeanStore& db)
{
   cout << name << " pages" << endl;
   u64 pages = adapter.btree->countPages();
   cout << "nodes:" << pages << " space:" << pages / (float)(db.getBufferManager().consumedPages() / 100) << "% height:" << adapter.btree->getHeight()
        << endl;
}

void printLoadStatistics(LeanStore& db)
{
   auto& crm = db.getCRManager();
   double gib = (db.getBufferManager().consumedPages() * EFFECTIVE_PAGE_SIZE / 1024.0 / 1024.0 / 1024.0);
   cout << "data loaded - consumed space in GiB = " << gib << endl;
   //    crm.scheduleJobSync(0, [&]() { printer<bf_record_t>("scaled", scaled_table, db); });
}

void startBenchmarkThreads(LeanStore& db, atomic<u64>& keep_running, u64 tx_per_thread[]);
void executeOneTx(volatile u64& tx_acc);
void stopBenchmarkThreads(LeanStore& db, atomic<u64>& keep_running);
void printBenchmarkStatistics(u64 tx_per_thread[]);

void run(LeanStore& db)
{
   atomic<u64> keep_running(true);
   atomic<u64> running_threads_counter(0);
   vector<thread> threads;
   u64 tx_per_thread[FLAGS_worker_threads];
   startBenchmarkThreads(db, keep_running, tx_per_thread);
   stopBenchmarkThreads(db, keep_running);
   printBenchmarkStatistics(tx_per_thread);
}

void startBenchmarkThreads(LeanStore& db, atomic<u64>& keep_running, u64 tx_per_thread[])
{
   auto& crm = db.getCRManager();
   db.startProfilingThread();
   crm.scheduleJobs(FLAGS_worker_threads, [&, tx_per_thread](u64 t_i) {
      volatile u64 tx_acc = 0;
      while (keep_running) {
         jumpmuTry()
         {
            executeOneTx(tx_acc);
         }
         jumpmuCatch()
         {
            WorkerCounters::myCounters().tx_abort++;
         }
      }
      tx_per_thread[t_i] = tx_acc;
   });
}

void executeOneTx(volatile u64& tx_acc)
{
   cr::Worker::my().startTX();
   runOneQuery();
   WorkerCounters::myCounters().tx++;
   tx_acc++;
}

void stopBenchmarkThreads(LeanStore& db, atomic<u64>& keep_running)
{
   auto& crm = db.getCRManager();
   sleep(FLAGS_run_for_seconds);
   keep_running = false;
   crm.joinAll();
}

void printBenchmarkStatistics(u64 tx_per_thread[])
{
   cout << endl;
   for (u64 t_i = 0; t_i < FLAGS_worker_threads; t_i++) {
      if (t_i != FLAGS_worker_threads - 1) {
         cout << tx_per_thread[t_i] << ", ";
      } else {
         cout << tx_per_thread[t_i] << endl;
      }
   }
   auto total_throughput = 0;
   for (u64 t_i = 0; t_i < FLAGS_worker_threads; t_i++) {
      total_throughput += tx_per_thread[t_i];
   }
   cout << "Total throughput: " << total_throughput << endl;
}

void printRunStatistics(LeanStore& db)
{
   double gib = (db.getBufferManager().consumedPages() * EFFECTIVE_PAGE_SIZE / 1024.0 / 1024.0 / 1024.0);
   cout << endl << "consumed space in GiB = " << gib << endl;
}
