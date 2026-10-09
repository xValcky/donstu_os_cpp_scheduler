#pragma once
#include "scheduler.h"
#include <algorithm>
#include <vector>

// Невытесняющий планировщик HRRN (Highest Response Ratio Next)
class HrrnScheduler : public Scheduler {
public:
  explicit HrrnScheduler(std::vector<Process> processes)
      : Scheduler(std::move(processes)) {}

  std::string name() const override { return "HRRN"; }

  void onTick(std::uint64_t tick) override {
    for (auto& p : processes_) {
      if (p.state == ProcessState::NEW && p.arrivalTime <= tick) {
        p.state = ProcessState::READY;
        ready_.push_back(p.pid);
      }
    }
  }

  void onProcessReady(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

  int pickNext(std::uint64_t) override {
    if (ready_.empty()) return -1;

    // Лямбда-функция для подсчета коэффициента отклика R
    auto ratio = [&](int pid) {
      const Process* p = find(pid);
      // Приводим к double, чтобы избежать целочисленного деления
      return static_cast<double>(p->waitingTime + p->burstTime) / p->burstTime;
    };

    // Ищем процесс с НАИБОЛЬШИМ коэффициентом отклика R
    auto it = std::max_element(ready_.begin(), ready_.end(),
                               [&](int a, int b) { return ratio(a) < ratio(b); });

    int pid = *it;
    ready_.erase(it);
    return pid;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
    auto it = std::find(ready_.begin(), ready_.end(), pid);
    if (it != ready_.end()) ready_.erase(it);
  }

  void onProcessBlocked(int, std::uint64_t) override {}

  void onProcessPreempted(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

  bool shouldPreempt(int, std::uint64_t) override {
    // Алгоритм невытесняющий
    return false;
  }

private:
  std::vector<int> ready_;
};
