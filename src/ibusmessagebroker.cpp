/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>
#include <algorithm>

#include "bus/ibusmessagebroker.h"

using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace std::chrono;

namespace bus {

std::shared_ptr<IBusMessageQueue> IBusMessageBroker::CreatePublisher() {
  auto publisher =
    std::make_shared<IBusMessageQueue>();
  publishers_.emplace_back(publisher);
  return publisher;
}

std::shared_ptr<IBusMessageQueue> IBusMessageBroker::CreateSubscriber() {
  auto subscriber = std::make_shared<IBusMessageQueue>();
  subscribers_.emplace_back(subscriber);
  return subscriber;
}

void IBusMessageBroker::DetachPublisher(
    const std::shared_ptr<IBusMessageQueue>& publisher) {
  auto itr = std::ranges::find_if(publishers_,
    [&] (const auto& pub) -> bool {
      return pub == publisher;
    });
  if (itr != publishers_.end()) {
    publishers_.erase(itr);
  }
}

void IBusMessageBroker::DetachSubscriber(
    const std::shared_ptr<IBusMessageQueue>& subscriber) {
  auto itr = std::ranges::find_if(subscribers_,
    [&] (const auto& sub) -> bool {
      return sub == subscriber;
    });
  if (itr != subscribers_.end()) {
    subscribers_.erase(itr);
  }
}

size_t IBusMessageBroker::NofPublishers() const {
  return publishers_.size();
}

size_t IBusMessageBroker::NofSubscribers() const {
  return subscribers_.size();
}

void IBusMessageBroker::Start() {
  // Just stop any on-going thread.
  // Should not happen so it's OK to generate an error.
  Stop();

  // Start the working thread
  stop_thread_ = false;
  thread_ = std::thread(&IBusMessageBroker::InprocessThread, this);

}

void IBusMessageBroker::Stop() {
  stop_thread_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
  stop_thread_ = false;
}

void IBusMessageBroker::Poll(IBusMessageQueue& queue) const {
  for (auto msg = queue.Pop(); msg && !stop_thread_;
      msg = queue.Pop()) {
    for (auto& subscriber : subscribers_) {
      if (!subscriber) {
        continue;
      }
      auto copy = msg;
      subscriber->Push(copy);
    }
  }
}

void IBusMessageBroker::InprocessThread() {
  while (!stop_thread_) {
    for (auto& publisher : publishers_) {
      if (stop_thread_ || !publisher) {
        continue;
      }
      Poll(*publisher);
    }
    sleep_for( 10ms);
  }

}

} // bus