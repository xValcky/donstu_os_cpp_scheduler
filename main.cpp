#include "fcfs.h"
#include "testsets.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "hrrn.h"
#include "simulator.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>

std::vector<Process> makeTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid;
    p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "P1", 0, 8, 3);
  add(2, "P2", 1, 4, 1);
  add(3, "P3", 2, 9, 4);
  add(4, "P4", 3, 5, 2);
  return procs;
}

std::vector<Process> makeIoTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name,
                 std::uint64_t arrival, std::uint64_t burst,
                 int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1",  1,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(3, "IO2",  2,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(4, "CPU2", 3, 10, 3);
  return procs;
}

std::vector<Process> makeConvoySet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1",  1,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
  add(3, "IO2",  2,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
  add(4, "CPU2", 3, 12, 3);
  return procs;
}

std::vector<Process> makeHrrnTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    procs.push_back(p);
  };
  add(1, "A",  0, 4, 1);
  add(2, "L",  1, 6, 1);
  add(3, "S1", 4, 2, 1);
  add(4, "S2", 5, 1, 1);
  return procs;
}

void printResult(const SimResult& r) {
  std::cout << std::left << std::setw(30) << r.algorithm
            << " | wait=" << std::setw(7) << std::fixed << std::setprecision(2) << r.avgWaiting
            << " | turn=" << std::setw(7) << r.avgTurnaround
            << " | resp=" << std::setw(7) << r.avgResponse
            << " | CPU=" << std::setw(6) << r.cpuUtilization << "%"
            << " | CS=" << std::setw(4) << r.contextSwitches
            << " | done=" << r.throughput
            << "\n";
}

void printGantt(const SimResult& r) {
  std::cout << "Gantt (" << r.algorithm << "):\n";
  for (auto& [pid, span] : r.gantt) {
    std::cout << "  [" << span.first << "-" << span.second << ") ";
    if (pid == -1) std::cout << "IDLE\n";
    else if (pid == -2) std::cout << "CS\n";
    else std::cout << "P" << pid << "\n";
  }
  std::cout << "\n";
}

int main() {
  std::cout << "Case 1: CPU-bound\n";
  {
    auto set1 = makeTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set1));
    scheds.push_back(std::make_unique<SjfScheduler>(set1));
    scheds.push_back(std::make_unique<SrtnScheduler>(set1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 2));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, false, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set1));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\nCase 2. Mixed loading: I/O + CPU\n";
  {
    auto set2 = makeIoTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set2));
    scheds.push_back(std::make_unique<SrtnScheduler>(set2));
    scheds.push_back(std::make_unique<RrScheduler>(set2, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set2, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set2));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\nПо процессам, RR (q=2):\n";
  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    runSimulation(rr);
    printProcessTable(rr.processes());
  }

  {
    auto set = makeTestSet();
    saveSet("set_basic.txt", set);
    std::vector<Process> loaded;
    loadSet("set_basic.txt", loaded);
    FcfsScheduler a(set), b(loaded);
    std::cout << "\nПроверка Задания 13:\n";
    printResult(runSimulation(a));
    printResult(runSimulation(b));
  }

  std::cout << "\nCase 3: SJF vs HRRN comparison\n";
  {
    auto hrrn_set1 = makeHrrnTestSet();
    auto hrrn_set2 = makeHrrnTestSet();
    SjfScheduler sjf(hrrn_set1);
    HrrnScheduler hrrn(hrrn_set2);
    printResult(runSimulation(sjf));
    printResult(runSimulation(hrrn));
  }

  std::cout << "\nCase 4: Context Switch Overhead (cost=1)\n";
  {
    auto set = makeTestSet();
    RrScheduler rr1(set, 1);
    RrScheduler rr2(set, 2);
    RrScheduler rr4(set, 4);
    RrScheduler rr8(set, 8);
    printResult(runSimulation(rr1, 100000, 1));
    printResult(runSimulation(rr2, 100000, 1));
    printResult(runSimulation(rr4, 100000, 1));
    printResult(runSimulation(rr8, 100000, 1));
  }

  {
    auto set = makeTestSet();
    std::cout << "\nВлияние кванта в RR (набор makeTestSet)\n";
    std::cout << "q   wait   turn   resp   CS\n";
    for (std::uint64_t q : {1, 2, 4, 8, 16}) {
      RrScheduler rr(set, q);
      SimResult r = runSimulation(rr);
      std::cout << std::setw(2) << q << "  " << std::fixed << std::setprecision(2)
                << r.avgWaiting << "  " << r.avgTurnaround << "  " << r.avgResponse
                << "  " << std::setw(3) << r.contextSwitches << "  "
                << std::string(r.contextSwitches, '#') << "\n";
    }
  }

  // ==========================================
  // ИСПРАВЛЕНО И РАСШИРЕНО ДЛЯ ЗАДАНИЯ 6
  // ==========================================
  std::cout << "\nCase 6: Convoy Effect Analysis\n";
  {
    auto set = makeConvoySet();
    
    FcfsScheduler fcfs(set);
    SrtnScheduler srtn(set);
    RrScheduler rr4(set, 4);
    PriorityScheduler prio(set, true, true);
    MlfqScheduler mlfq(set);

    std::cout << "\n--- Сводные результаты алгоритмов ---\n";
    printResult(runSimulation(fcfs));
    printResult(runSimulation(srtn));
    printResult(runSimulation(rr4));
    printResult(runSimulation(prio));
    printResult(runSimulation(mlfq));

    std::cout << "\n--- Таблица по процессам: FCFS ---\n";
    printProcessTable(fcfs.processes());

    std::cout << "\n--- Таблица по процессам: MLFQ ---\n";
    printProcessTable(mlfq.processes());
  }

  return 0;
}
