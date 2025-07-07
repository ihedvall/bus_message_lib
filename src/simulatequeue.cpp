/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include "simulatequeue.h"

#include <chrono>

#include "bus/buslogstream.h"
#include "simulatebroker.h"

using namespace std::chrono_literals;

namespace bus {

SimulateQueue::SimulateQueue(SimulateBroker& broker, bool publisher)
  : broker_(broker),
    publisher_(publisher) {

}

SimulateQueue::~SimulateQueue() {
  SimulateQueue::Stop();
}

void SimulateQueue::Start() {
  Stop();
  stop_thread_ = false;
  if (publisher_) {
    thread_ = std::thread(&SimulateQueue::PublisherTask, this);
  } else {
    broker_.GetChannel(*this);
    thread_ = std::thread(&SimulateQueue::SubscriberTask, this);
  }
}

void SimulateQueue::Stop() {
  stop_thread_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
  stop_thread_ = false;
}

void SimulateQueue::PublisherTask() {
  while (!stop_thread_ ) {
    while (!broker_.BufferFull() && !Empty()) {
      broker_.PublisherPoll(*this);
      std::this_thread::yield();
    }
    std::this_thread::sleep_for(10ms);
  }
}

void SimulateQueue::SubscriberTask() {
  while (!stop_thread_) {
    while (broker_.SubscriberPoll(*this) ) {
      std::this_thread::yield();
    }
    std::this_thread::sleep_for(10ms);
  }
}
} // bus