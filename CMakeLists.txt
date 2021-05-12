cmake_minimum_required(VERSION 3.12)
project(ipm VERSION 2.1.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(logging REQUIRED)

find_package(cppzmq REQUIRED)
find_package(ers REQUIRED)
find_package(nlohmann_json REQUIRED)


daq_add_library(Receiver.cpp Sender.cpp LINK_LIBRARIES appfwk::appfwk logging::logging cppzmq)

daq_codegen(*.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

daq_add_plugin(ZmqSender duneIPM LINK_LIBRARIES ipm)
daq_add_plugin(ZmqReceiver duneIPM LINK_LIBRARIES ipm)
daq_add_plugin(ZmqPublisher duneIPM LINK_LIBRARIES ipm)
daq_add_plugin(ZmqSubscriber duneIPM LINK_LIBRARIES ipm)

daq_add_plugin(VectorIntIPMSenderDAQModule     duneDAQModule TEST LINK_LIBRARIES ipm )
daq_add_plugin(VectorIntIPMReceiverDAQModule   duneDAQModule TEST LINK_LIBRARIES ipm )
daq_add_plugin(VectorIntIPMSubscriberDAQModule duneDAQModule TEST LINK_LIBRARIES ipm)
add_dependencies(ipm_VectorIntIPMSubscriberDAQModule_duneDAQModule ipm_VectorIntIPMReceiverDAQModule_duneDAQModule)

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


daq_install()
