//
// Created by Dell on 25-06-2025.
//
#include <gtest/gtest.h>
#include <util/timestamp.h>
#include <util/utilfactory.h>

#include "bus/buslogstream.h"
#include "bus/candataframe.h"
#include "bus/interface/businterfacefactory.h"
#include "bustolisten.h"

using namespace std::chrono;

namespace bus {

TEST(BusMessage, Mainfunc1) {
  // create a broker
  auto broker =
      BusInterfaceFactory::CreateBroker(BrokerType::SharedMemoryBrokerType);
  broker->Name("SharedMemoryBroker");
  broker->Start();
  auto broker_publisher = broker->CreatePublisher();
  broker_publisher->Start();
  BusToListen bus_to_listen;
  bus_to_listen.args_.push_back("BusMessage");
  // create a thread to run main method
  std::thread broker_thread = std::thread(&BusToListen::MainFunc, &bus_to_listen);
  auto msgPtr = std::make_shared<CanDataFrame>();
  auto timestamp = util::time::TimeStampToNs();
  msgPtr->Timestamp(timestamp);
  msgPtr->BusChannel(1);
  std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
  msgPtr->DataBytes(data);


  broker_publisher->Push(msgPtr);
  int count = 0;
  while ( count < 50) {
    std::this_thread::sleep_for(100ms);
    count++;
  }
  BusToListen::StopMessage();
  broker_thread.join();
}

}  // namespace bus