/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>
#include <algorithm>
#include <mutex>
#include <vector>

#include "bus/ibusmessagebroker.h"

using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace std::chrono;

namespace bus {

void IBusMessageBroker::Name(std::string name) {
  name_ = std::move(name);
}

const std::string& IBusMessageBroker::Name() const {
  return name_;
}

void IBusMessageBroker::Address(std::string address) {
  address_ = std::move(address);
}

const std::string& IBusMessageBroker::Address() const {
  return address_;
}

bool IBusMessageBroker::IsConnected() const {
  return connected_;
}

std::shared_ptr<IBusMessageQueue> IBusMessageBroker::CreatePublisher() {
  auto publisher = std::make_shared<IBusMessageQueue>();

  std::lock_guard queue_lock(queue_mutex_);
  publishers_.emplace_back(publisher);

  return publisher;
}

std::shared_ptr<IBusMessageQueue> IBusMessageBroker::CreateSubscriber() {
  auto subscriber = std::make_shared<IBusMessageQueue>();

  std::lock_guard queue_lock(queue_mutex_);
  subscribers_.emplace_back(subscriber);

  return subscriber;
}

void IBusMessageBroker::DetachPublisher(
    const std::shared_ptr<IBusMessageQueue>& publisher) {
  std::lock_guard queue_lock(queue_mutex_);
  std::erase_if(publishers_,[&] (const auto& pub) -> bool {
      return pub.get() == publisher.get();
    });
}

void IBusMessageBroker::DetachSubscriber(
    const std::shared_ptr<IBusMessageQueue>& subscriber) {
  std::lock_guard queue_lock(queue_mutex_);
  std::erase_if (subscribers_, [&] (const auto& sub) -> bool {
      return sub.get() == subscriber.get();
    });
}

size_t IBusMessageBroker::NofPublishers() const {
  std::lock_guard queue_lock(queue_mutex_);
  return publishers_.size();
}

size_t IBusMessageBroker::NofSubscribers() const {
  std::lock_guard queue_lock(queue_mutex_);
  return subscribers_.size();
}

void IBusMessageBroker::Start() {
  // Just stop any on-going thread.
  // Should not happen so it's OK to generate an error.
  Stop();

  // Start the working thread
  stop_thread_ = false;
  thread_ = std::thread(&IBusMessageBroker::InprocessThread, this);
  connected_ = true;
}

void IBusMessageBroker::Stop() {
  connected_ = false;
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

void IBusMessageBroker::InprocessThread() const {
  while (!stop_thread_) {
    {
      std::scoped_lock queue_lock(queue_mutex_);
      for (auto& publisher : publishers_) {
        if (stop_thread_ || !publisher) {
          continue;
        }
        Poll(*publisher);
      }
    }
    sleep_for( 10ms);
  }

}

} // bus