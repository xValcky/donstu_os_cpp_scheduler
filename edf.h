#pragma once
#include "scheduler.h"
#include <algorithm>
#include <vector>

// Earliest Deadline First (планировщик реального времени)
class EdfScheduler : public Scheduler {
public:
  using Scheduler::Scheduler;

  std::string name() const override { return "EDF"; }

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

    // Шаг 1.3: Выбираем процесс с НАИМЕНЬШИМ абсолютным дедлайном
    auto it = std::min_element(ready_.begin(), ready_.end(), [this](int a, int b) {
      Process* pA = find(a);
      Process* pB = find(b);
      std::uint64_t dlA = pA ? pA->deadline : 999999;
      std::uint64_t dlB = pB ? pB->deadline : 999999;
      return dlA < dlB;
    });

    int pid = *it;
    ready_.erase(it); // Извлекаем из очереди готовых
    return pid;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED; // Обязательно завершаем!
  }

  void onProcessBlocked(int /*pid*/, std::uint64_t) override {
    // вернётся через onProcessReady
  }

  void onProcessPreempted(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

  bool shouldPreempt(int currentPid, std::uint64_t /*tick*/) override {
    if (ready_.empty()) return false;

    Process* cur = find(currentPid);
    if (!cur || cur->isFinished()) return false;

    // Шаг 1.4: Вытесняем, если у кого-то в READY дедлайн СТРОГО меньше текущего
    for (int pid : ready_) {
      Process* readyProc = find(pid);
      if (readyProc && readyProc->deadline < cur->deadline) {
        return true;
      }
    }
    return false;
  }

private:
  std::vector<int> ready_;
};
