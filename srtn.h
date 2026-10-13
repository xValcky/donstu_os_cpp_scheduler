#pragma once
#include "scheduler.h"
#include <algorithm>
#include <vector>

// Shortest Remaining Time Next (вытесняющий SJF)
class SrtnScheduler : public Scheduler {
public:
  using Scheduler::Scheduler;

  std::string name() const override { return "SRTN"; }

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

  int pickNext(std::uint64_t /*tick*/) override {
    if (ready_.empty()) return -1;

    // Ищем процесс с минимальным оставшимся временем
    auto it = std::min_element(ready_.begin(), ready_.end(), [this](int a, int b) {
      Process* pA = find(a);
      Process* pB = find(b);
      std::uint64_t remA = pA ? pA->remainingTime : 999999;
      std::uint64_t remB = pB ? pB->remainingTime : 999999;
      return remA < remB;
    });

    int pid = *it;
    ready_.erase(it); // Удаляем, чтобы другие ядра не перехватили
    return pid;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
  }

  void onProcessBlocked(int /*pid*/, std::uint64_t) override {
    // вернётся через onProcessReady
  }

  void onProcessPreempted(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

 bool shouldPreempt(int pid, std::uint64_t /*tick*/) override {
    if (ready_.empty()) return false;

    Process* currentProc = find(pid);
    if (!currentProc) return false;

    // Ищем минимальное оставшееся время среди готовых
    std::uint64_t minReadyRem = 999999;
    for (int r_pid : ready_) {
      Process* p = find(r_pid);
      if (p && p->remainingTime < minReadyRem) {
        minReadyRem = p->remainingTime;
      }
    }

    // Вытесняем, только если процесс в очереди строго короче текущего
    return minReadyRem < currentProc->remainingTime;
  } 

private:
  std::vector<int> ready_;
};
