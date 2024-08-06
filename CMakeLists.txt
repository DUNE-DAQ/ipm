cmake_minimum_required(VERSION 3.12)
project(ipm VERSION 2.7.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(logging REQUIRED)
find_package(utilities REQUIRED)
find_package(cetlib REQUIRED)
find_package(cppzmq REQUIRED)
find_package(ers REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost COMPONENTS unit_test_framework program_options REQUIRED)
find_package(opmonlib REQUIRED)


set(IPM_DEPENDENCIES ${CETLIB} ${CETLIB_EXCEPT} ers::ers logging::logging nlohmann_json::nlohmann_json utilities::utilities opmonlib::opmonlib cppzmq pthread)

daq_protobuf_codegen( opmon/IPMInfo.proto )

daq_add_library(Receiver.cpp Sender.cpp CallbackAdapter.cpp LINK_LIBRARIES ${IPM_DEPENDENCIES})

daq_add_plugin(ZmqSender duneIPM LINK_LIBRARIES ipm)
daq_add_plugin(ZmqReceiver duneIPM LINK_LIBRARIES ipm)
daq_add_plugin(ZmqPublisher duneIPM LINK_LIBRARIES ipm)
daq_add_plugin(ZmqSubscriber duneIPM LINK_LIBRARIES ipm)

daq_add_unit_test(Sender_test LINK_LIBRARIES ipm)
daq_add_unit_test(Receiver_test LINK_LIBRARIES ipm)
daq_add_unit_test(Subscriber_test LINK_LIBRARIES ipm)

daq_add_unit_test(ZmqSender_test LINK_LIBRARIES ipm)
daq_add_unit_test(ZmqReceiver_test LINK_LIBRARIES ipm)
daq_add_unit_test(ZmqPublisher_test LINK_LIBRARIES ipm)
daq_add_unit_test(ZmqSubscriber_test LINK_LIBRARIES ipm)
daq_add_unit_test(ZmqSendReceive_test LINK_LIBRARIES ipm)
daq_add_unit_test(ZmqPubSub_test LINK_LIBRARIES ipm)
set_tests_properties(ZmqSender_test ZmqReceiver_test ZmqPublisher_test ZmqSubscriber_test ZmqSendReceive_test ZmqPubSub_test PROPERTIES ENVIRONMENT "CET_PLUGIN_PATH=${CMAKE_CURRENT_BINARY_DIR}/plugins:$ENV{CET_PLUGIN_PATH}")

daq_add_application(zmq_send zmq_send.cpp TEST LINK_LIBRARIES ipm Boost::program_options)
daq_add_application(zmq_recv zmq_recv.cpp TEST LINK_LIBRARIES ipm Boost::program_options)

daq_install()
