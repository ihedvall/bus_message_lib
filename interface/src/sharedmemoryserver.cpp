/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>
#include <algorithm>

#include <boost/interprocess/sync/scoped_lock.hpp>

#include "sharedmemoryserver.h"

#include "sharedmemorytxrxqueue.h"
#include "bus/buslogstream.h"

using namespace std::chrono_literals;
using namespace boost::interprocess;

namespace bus {
SharedMemoryServer::~SharedMemoryServer() {
  SharedMemoryServer::Stop();
}


void SharedMemoryServer::Start() {
  Stop(); // Stop on-going threads
  if (Name().empty()) {
    Name("BusMessageServer");
  }
  shared_memory_object::remove(Name().c_str());
  ConnectToSharedMemory();

  if (shm_ != nullptr) {
    stop_server_threads_ = false;
    tx_thread_ = std::thread(&SharedMemoryServer::TxThread, this);
    rx_thread_ = std::thread(&SharedMemoryServer::RxThread, this);
    connected_ = true;
  } else {
    connected_ = false;
  }
  std::this_thread::yield();
}

void SharedMemoryServer::Stop() {
  connected_ = false;
  if (shm_ == nullptr) {
    return;
  }
  stop_server_threads_ = true;
  shm_->tx_full_condition.notify_all(); // Speed up the stop
  shm_->rx_full_condition.notify_all(); // Speed up the stop
  if (tx_thread_.joinable()) {
    tx_thread_.join();
  }
  if (rx_thread_.joinable()) {
    rx_thread_.join();
  }
  shm_ = nullptr;
  region_.reset();
  shared_memory_.reset();
  shared_memory_object::remove(Name().c_str());
}

std::shared_ptr<IBusMessageQueue> SharedMemoryServer::CreatePublisher() {
  auto publisher = std::make_shared<SharedMemoryTxRxQueue>(Name(),
    true, true);
  return publisher;
}

std::shared_ptr<IBusMessageQueue> SharedMemoryServer::CreateSubscriber() {
  auto subscriber = std::make_shared<SharedMemoryTxRxQueue>(Name(),
    false, false);
  return subscriber;
}

void SharedMemoryServer::ConnectToSharedMemory() {
  shm_ = nullptr;
  region_.reset();
  shared_memory_.reset();

  shared_memory_object::remove(Name().c_str());

  try {
    shared_memory_ = std::make_unique<shared_memory_object>(
        create_only, Name().c_str(), read_write);
    if (!shared_memory_) {
      std::ostringstream err;
      err << "Failed to create shared memory. Name: " << Name();
      throw std::runtime_error(err.str());
    }
    shared_memory_->truncate(sizeof(SharedServerObjects));

    region_ = std::make_unique<mapped_region>(*shared_memory_, read_write);
    if (!region_ || region_->get_address() == nullptr) {
      std::ostringstream err;
      err << "Failed to get a region address. Name: " << Name();
      throw std::runtime_error(err.str());
    }
    std::memset(region_->get_address(), 0, region_->get_size());
    shm_ = new(region_->get_address()) SharedServerObjects();

    scoped_lock lock(shm_->memory_mutex);
    // Reset the channels in/out indexes.
    std::ranges::for_each(shm_->tx_channels, [] (Channel& channel) ->void {
      channel.used = false;
      channel.queue_index = 0;
    });
    std::ranges::for_each(shm_->rx_channels, [] (Channel& channel) ->void {
      channel.used = false;
      channel.queue_index = 0;
    });

    // Allocate the first array item for the publishers.
    // The other array items are used for the subscribers.
    // The subscriber tries to allocate a free channal
    // array at startup.
    shm_->tx_channels[0].used = true;
    shm_->rx_channels[0].used = true;
    shm_->initialized = true;

  } catch (std::exception &err) {
    BUS_ERROR() << "Failed to create the shared memory. Name: " << Name()
      << " Error: " << err.what();
    shm_ = nullptr;
    region_.reset();
    shared_memory_.reset();
    shared_memory_object::remove(Name().c_str());
  }
}

void SharedMemoryServer::TxThread() {
  while (!stop_server_threads_ || shm_ == nullptr) {
    scoped_lock lock(shm_->memory_mutex);
    shm_->tx_full_condition.wait_for(lock, 100ms, [&] () -> bool {
      return stop_server_threads_.load() || shm_->tx_full.load();
    } );
    if (stop_server_threads_) {
      return;
    }
    if (shm_->tx_full) {
      HandleTxFull();
    }
  }
}

void SharedMemoryServer::HandleTxFull() {
  if (shm_ == nullptr) {
    return;
  }
  const uint32_t ref_index  = shm_->tx_channels[0].queue_index;

  bool all_index_ok = std::ranges::all_of( shm_->tx_channels,
    [&] (const Channel& channel)-> bool {
      if (!channel.used) {
        return true;
      }
      return channel.queue_index == ref_index;
    });

  if (all_index_ok) {
    ResetTxChannels();
  } else if (shm_->tx_full) {
    std::time_t now = std::time(nullptr);
    if (tx_timeout_ == 0) {
      tx_timeout_ = now + 10;;
    } else if (now > tx_timeout_) {
      BUS_ERROR() << "TX buffer full (10s) timeout occurred. Resetting";
      ResetTxChannels();
    }
  }
}

void SharedMemoryServer::ResetTxChannels() {
  if (shm_ == nullptr) {
    return;
  }
  std::ranges::for_each(shm_->tx_channels, [] (Channel& channel) -> void {
    channel.queue_index = 0;
  });

  shm_->tx_full= false;
  tx_timeout_ = 0;
}

void SharedMemoryServer::RxThread() {
  while (!stop_server_threads_ || shm_ == nullptr) {
    scoped_lock lock(shm_->memory_mutex);
    shm_->rx_full_condition.wait_for(lock, 100ms, [&] () -> bool {
      return stop_server_threads_.load() || shm_->rx_full.load();
    } );
    if (stop_server_threads_) {
      return;
    }
    if (shm_->rx_full) {
      HandleRxFull();
    }
  }
}

void SharedMemoryServer::HandleRxFull() {
  if (shm_ == nullptr) {
    return;
  }
  const uint32_t ref_index  = shm_->rx_channels[0].queue_index;

  bool all_index_ok = std::ranges::all_of( shm_->rx_channels,
    [&] (const Channel& channel)-> bool {
      if (!channel.used) {
        return true;
      }
      return channel.queue_index == ref_index;
    });

  if (all_index_ok) {
    ResetRxChannels();
  } else if (shm_->rx_full) {
    std::time_t now = std::time(nullptr);
    if (rx_timeout_ == 0) {
      rx_timeout_ = now + 10;;
    } else if (now > rx_timeout_) {
      BUS_ERROR() << "RX buffer full (10s) timeout occurred. Resetting";
      ResetRxChannels();
    }
  }
}

void SharedMemoryServer::ResetRxChannels() {
  if (shm_ == nullptr) {
    return;
  }
  std::ranges::for_each(shm_->rx_channels, [] (Channel& channel) -> void {
    channel.queue_index = 0;
  });

  shm_->rx_full= false;
  rx_timeout_ = 0;
}
} // bus