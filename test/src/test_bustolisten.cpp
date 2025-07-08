//
// Created by Dell on 25-06-2025.
//
#include <gtest/gtest.h>
#include <listenconsole.h>
#include <util/timestamp.h>
#include <util/utilfactory.h>

#include "bus/buslogstream.h"
#include "bus/candataframe.h"
#include "bus/interface/businterfacefactory.h"
#include "bustolisten.h"

using namespace std::chrono;

namespace bus {
class BusListen : public util::log::detail::ListenConsole {
 public:
  bool message_received = false;
  BusListen(const std::string &share_name) : ListenConsole(share_name) {}
  void AddMessage(uint64_t nano_sec_1970, const std::string &pre_text,
                  const std::string &text) override {
    std::cout << "Message received: " << text << std::endl;
    message_received = true;
  }
};


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
  //util::log::detail::ListenConsole listen_console("LISBUS");
  //BusListen listen_server("LISBUS");
 // listen_server.Start();
  //listen_console.Start();
// exit the loop
  int count = 0;
  while ( count < 50) {
    std::this_thread::sleep_for(100ms);
    count++;
  }
 // EXPECT_TRUE(listen_console.GetNumberOfMessages() > 0) << "Message not Received";

  BusToListen::StopMessage();
 // listen_console.Stop();
  broker_thread.join();
}

}  // namespace bus