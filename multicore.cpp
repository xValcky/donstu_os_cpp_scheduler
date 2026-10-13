#include "simulator.h"
#include <vector>
#include <algorithm>
#include <iostream>

SimResult runSimulationMulti(Scheduler& sched, int cores, std::uint64_t maxTicks) {
  SimResult res;
  res.algorithm = sched.name() + " x" + std::to_string(cores);
  res.coreGantt.resize(cores);

  std::vector<int> current(cores, -1);  // КТО сейчас выполняется на каждом ядре
  std::vector<int> prev(cores, -1);     // КТО выполнялся на ядре на прошлом такте
  std::uint64_t busyTicks = 0;          // Суммарное полезное время работы всех ядер
  std::uint64_t tick = 0;
  auto& procs = sched.processes();

  while (tick < maxTicks) {
    // 1. Возврат из ввода-вывода (I/O)
    for (auto& p : procs) {
      if (p.state == ProcessState::WAITING && tick >= p.ioReturnTick) {
        p.state = ProcessState::READY;
        sched.onTick(tick); // Уведомляем планировщик
      }
    }

    // 2. Проверяем, все ли процессы завершены
    bool allTerminated = true;
    for (const auto& p : procs) {
      if (p.state != ProcessState::TERMINATED) {
        allTerminated = false;
        break;
      }
    }
    if (allTerminated) {
      break;
    }

    // 3. Вызов общего onTick один раз за такт (для добавления новых процессов)
    sched.onTick(tick);

    // 4. Вытеснение: для каждого активного ядра проверяем shouldPreempt
    for (int c = 0; c < cores; ++c) {
      int pid = current[c];
      if (pid != -1) {
        if (sched.shouldPreempt(pid, tick)) {
          Process* p = sched.find(pid);
          if (p && p->state == ProcessState::RUNNING) {
            p->state = ProcessState::READY;
		sched.onProcessPreempted(pid, tick); // ВОТ ЭТА СТРОКА НЕОБХОДИМА!
          }
          current[c] = -1; // Освобождаем ядро
        }
      }
    }

    // 5. Распределение задач: каждое свободное ядро пытается взять следующий процесс
    for (int c = 0; c < cores; ++c) {
      if (current[c] == -1) {
        int nextPid = sched.pickNext(tick);
        if (nextPid != -1) {
          current[c] = nextPid;
          Process* p = sched.find(nextPid);
          if (p) {
            p->state = ProcessState::RUNNING;
            // Исправление ошибки №1 из Части 0: фиксируем время первого отклика
            if (!p->started) {
              p->started = true;
              p->startTime = tick;
            }
          }
        }
      }
    }

    // 6. Выполнение такта на каждом ядре
    for (int c = 0; c < cores; ++c) {
      int pid = current[c];
      if (pid != -1) {
        Process* p = sched.find(pid);
        if (p) {
          p->remainingTime--;
          busyTicks++;

          // Проверка на уход в I/O
          bool goToIo = false;
          if (p->nextIoIndex < p->ioBlocks.size()) {
            std::uint64_t executed = p->burstTime - p->remainingTime;
            if (executed == p->ioBlocks[p->nextIoIndex].atTick) {
              goToIo = true;
            }
          }

          if (goToIo) {
            p->state = ProcessState::WAITING;
            p->ioReturnTick = tick + 1 + p->ioBlocks[p->nextIoIndex].duration; // Исправление №3
            p->ioWaitTime += p->ioBlocks[p->nextIoIndex].duration;
            p->nextIoIndex++;
            sched.onProcessBlocked(pid, tick);
            current[c] = -1; // Освобождаем ядро
          } 
          // Проверка на завершение процесса
          else if (p->remainingTime == 0) {
            p->state = ProcessState::TERMINATED;
            p->finishTime = tick + 1;
            p->turnaroundTime = p->finishTime - p->arrivalTime;
            p->responseTime = p->startTime - p->arrivalTime;
            sched.onProcessFinished(pid, tick);
            current[c] = -1; // Освобождаем ядро
          }
        }
      }
    }

    // 7. Накопление времени ожидания для всех, кто остался вREADY в конце такта
    for (auto& p : procs) {
      if (p.state == ProcessState::READY) {
        p.waitingTime++;
      }
    }

    // 8. Запись диаграммы Ганта отдельно по каждому ядру
    for (int c = 0; c < cores; ++c) {
      int ranPid = current[c];
      if (ranPid == -1 && prev[c] != -1) {
        // Если ядро работало, а теперь простаивает — ищем, не работает ли этот процесс на другом ядре
        // (в простейшей схеме без накладных расходов пишем просто idle)
      }
      
      if (!res.coreGantt[c].empty() && res.coreGantt[c].back().first == ranPid) {
        res.coreGantt[c].back().second.second = tick + 1;
      } else {
        res.coreGantt[c].push_back({ranPid, {tick, tick + 1}});
      }
      prev[c] = ranPid;
    }

    tick++;
  }

  // Сбор итоговых метрик по симуляции
  res.totalTicks = tick;
  if (tick > 0 && cores > 0) {
    res.cpuUtilization = 100.0 * busyTicks / (tick * cores);
  } else {
    res.cpuUtilization = 0.0;
  }

  // Расчет средних величин по всем процессам
  double totalWait = 0, totalTurn = 0, totalResp = 0;
  for (const auto& p : procs) {
    totalWait += p.waitingTime;
    totalTurn += p.turnaroundTime;
    totalResp += p.responseTime;
    if (p.state == ProcessState::TERMINATED) {
      res.throughput++;
    }
  }
  if (!procs.empty()) {
    res.avgWaiting = totalWait / procs.size();
    res.avgTurnaround = totalTurn / procs.size();
    res.avgResponse = totalResp / procs.size();
  }

  return res;
}
