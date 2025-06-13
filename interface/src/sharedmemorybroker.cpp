/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>
#include <algorithm>
#include <sstream>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "sharedmemorybroker.h"
#include "bus/buslogstream.h"

#include "sharedmemoryqueue.h"

using namespace std::chrono_literals;
using namespace boost::interprocess;
namespace bus {

SharedMemoryBroker::SharedMemoryBroker()
  : IBusMessageBroker() {

}

SharedMemoryBroker::~SharedMemoryBroker() {
  SharedMemoryBroker::Stop();
}

void SharedMemoryBroker::Start() {
  Stop(); // Stop on-going threads
  if (Name().empty()) {
    Name("BusMessageMaster");
  }

  try {
    shared_memory_object::remove(Name().c_str());
    shared_memory_ = std::make_unique<shared_memory_object>(
        create_only, Name().c_str(), read_write);
    if (!shared_memory_) {
      std::ostringstream err;
      err << "Failed to create shared memory. Name: " << Name();
      throw std::runtime_error(err.str());
    }
    shared_memory_->truncate(sizeof(SharedMemoryObjects));

    region_ = std::make_unique<mapped_region>(*shared_memory_, read_write);
    if (!region_ || region_->get_address() == nullptr) {
      std::ostringstream err;
      err << "Failed to get a region address. Name: " << Name();
      throw std::runtime_error(err.str());
    }
    std::memset(region_->get_address(), 0, region_->get_size());
    shm_ = new(region_->get_address()) SharedMemoryObjects();

    scoped_lock lock(shm_->memory_mutex);
    // Reset the channels in/out indexes.
    std::ranges::for_each(shm_->channels, [] (Channel& channel) ->void {
      channel.used = false;
      channel.queue_index = 0;
    });


    // Allocate the first array item for the publishers.
    // The other array items are used for the subscribers.
    // The subscriber tries to allocate a free channal
    // array at startup.
    shm_->channels[0].used = true;
    shm_->initialized = true;
    BUS_INFO() << "Shared memory initialized. Name: " << Name();
  } catch (std::exception &err) {
    BUS_ERROR() << "Failed to create the shared memory. Name: " << Name();
    shared_memory_object::remove(Name().c_str());
    return;
  }

  stop_master_task_ = false;
  master_task_ = std::thread(&SharedMemoryBroker::BrokerMasterTask, this);
  std::this_thread::yield();
}

void SharedMemoryBroker::Stop() {
  if (shm_ == nullptr) {
    return;
  }
  stop_master_task_ = true;
  shm_->buffer_full_condition.notify_all(); // Speed up the stop

  if (master_task_.joinable()) {
    master_task_.join();
  }

  shm_ = nullptr;
  region_.reset();
  shared_memory_.reset();
  shared_memory_object::remove(Name().c_str());
}

std::shared_ptr<IBusMessageQueue> SharedMemoryBroker::CreatePublisher() {
  std::shared_ptr<IBusMessageQueue> pub;
  if (!Name().empty()) {
    auto shm = std::make_shared<SharedMemoryQueue>(Name(), true);
    if (shm) {
      pub = shm;
    }
  }
  // No need to add the message queue to a list;
  return pub;
}

std::shared_ptr<IBusMessageQueue> SharedMemoryBroker::CreateSubscriber() {
  std::shared_ptr<IBusMessageQueue> sub;
  if (!Name().empty()) {
    auto shm = std::make_shared<SharedMemoryQueue>(Name(), false);
    if (shm) {
      sub = shm;
    }
  }
  // No need to add the message queue to a list;
  return sub;
}

void SharedMemoryBroker::BrokerMasterTask() {
  while (!stop_master_task_ || shm_ == nullptr) {
    scoped_lock lock(shm_->memory_mutex);
    shm_->buffer_full_condition.wait_for(lock, 1000ms, [&] () -> bool {
      return stop_master_task_.load() || shm_->buffer_full.load();
    } );
    if (!stop_master_task_ && shm_->buffer_full) {
      HandleBufferFull();
    } else {
      std::this_thread::sleep_for(100ms);
    }
  }
}

void SharedMemoryBroker::HandleBufferFull() {
  if (shm_ == nullptr) {
    return;
  }
  const uint32_t ref_index  = shm_->channels[0].queue_index;
  uint32_t out_index  = 0;
  bool all_index_ok = std::ranges::all_of( shm_->channels,
    [&] (const Channel& channel)-> bool {
      if (!channel.used) {
        return true;
      }
       out_index = channel.queue_index;
      return channel.queue_index == ref_index;
    });

  if (all_index_ok) {
    ResetChannels();
  } else if (shm_->buffer_full) {
    std::time_t now = std::time(nullptr);
    if (timeout_ == 0) {
      timeout_ = now + 10;;
    } else if (now > timeout_) {
      BUS_ERROR() << "Buffer full (10s) timeout occurred. Resetting";
      ResetChannels();
    }
  }
}

void SharedMemoryBroker::ResetChannels() {
  if (shm_ == nullptr) {
    return;
  }
  std::ranges::for_each(shm_->channels, [] (Channel& channel) -> void {
    channel.queue_index = 0;
  });
  shm_->buffer_full= false;
  timeout_ = 0;
}


} // bus