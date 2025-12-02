/*********************************************************************
* Acquire data continuously from an analog input channel using moniker based streaming
*
* The gRPC API is built from the C API. NI-DAQmx documentation is installed with the driver at:
*   C:\Program Files (x86)\National Instruments\NI-DAQ\docs\cdaqmx.chm
*
* Getting Started:
*
* To run this example, install "NI-DAQmx Driver" on the server machine:
*   https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html
*
* For instructions on how to use protoc to generate gRPC client interfaces, see our "Creating a gRPC
* Client" wiki page:
*   https://github.com/ni/grpc-device/wiki/Creating-a-gRPC-Client
*
* Refer to the NI DAQmx gRPC Wiki for the latest C Function Reference:
*   https://github.com/ni/grpc-device/wiki/NI-DAQMX-C-Function-Reference
*
* To run this example without hardware: create a simulated device in NI MAX on the server (Windows
* only).
*
* Build:
*
*   > mkdir build
*   > cd build
*   > cmake ..
*   > cmake --build .
*
* Running from command line:
*
* Server machine's IP address, port number, and physical channel name can be passed as separate
* command line arguments.
*   > MonikerStreamingClient <server_address> <port_number> <physical_channel_name>
* If they are not passed in as command line arguments, then by default the server address will be
* "localhost:31763", with "Dev1/ai0" as the physical channel name..
*********************************************************************/

#include <iostream>
#include <sstream>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <sideband_grpc.h>


#include "nidaqmx.grpc.pb.h"
#include "data_moniker.grpc.pb.h"

using namespace nidaqmx_grpc;
using StubPtr = std::unique_ptr<NiDAQmx::Stub>;


std::string SERVER_ADDRESS = "localhost";
std::string SERVER_PORT = "31763";
std::string PHYSICAL_CHANNEL_READ = "Dev1/ai0:7";
std::string PHYSICAL_CHANNEL_WRITE = "Dev1/ao0:7";
std::string READ_DEVICE = "PXI1Slot6";
std::string WRITE_DEVICE = "PXI1Slot6";
int NUM_CHANNELS = 4;
int NUM_ITERATIONS = 10000;

class grpc_driver_error : public std::runtime_error {
 private:
  ::grpc::StatusCode code_;
  std::multimap<std::string, std::string> trailers_;

 public:
  grpc_driver_error(const std::string& message, ::grpc::StatusCode code, const std::multimap<grpc::string_ref, grpc::string_ref>& trailers)
    : std::runtime_error(message), code_(code)
  {
    for (const auto& trailer : trailers) {
      trailers_.emplace(
        std::string(trailer.first.data(), trailer.first.length()),
        std::string(trailer.second.data(), trailer.second.length()));
    }
  }

  ::grpc::StatusCode StatusCode() const
  {
    return code_;
  }

  const std::multimap<std::string, std::string>& Trailers() const
  {
    return trailers_;
  }
};

inline void raise_if_error(const ::grpc::Status& status, const ::grpc::ClientContext& context)
{
  if (!status.ok()) {
    throw grpc_driver_error(status.error_message(), status.error_code(), context.GetServerTrailingMetadata());
  }
}

void print_array(const MonikerReadAnalogF64Response& data)
{
  
  std::cout << "Array Size: " << data.read_array().size() << " ";
  std::cout << "[";
    for (int i = 0; i < data.read_array().size(); i++) {
    std::cout << data.read_array().Get(i) << " ";
  }
  std::cout << "]" << std::endl;
}

::nidevice_grpc::Session create_and_configure_read_task(NiDAQmx::Stub &client, const std::string &PHYSICAL_CHANNEL_READ)
{
  ::grpc::ClientContext create_task_context;
  auto create_task_request = CreateTaskRequest{};
  create_task_request.set_session_name("my read task");
  auto create_task_response = CreateTaskResponse{};
  raise_if_error(
    client.CreateTask(&create_task_context, create_task_request, &create_task_response),
    create_task_context);
  auto daqmx_read_task = create_task_response.task();

  ::grpc::ClientContext create_channel_context;
  auto create_channel_request = CreateAIVoltageChanRequest{};
  create_channel_request.mutable_task()->CopyFrom(daqmx_read_task);
  create_channel_request.set_physical_channel(PHYSICAL_CHANNEL_READ);
  create_channel_request.set_terminal_config(InputTermCfgWithDefault::INPUT_TERM_CFG_WITH_DEFAULT_CFG_DEFAULT);
  create_channel_request.set_min_val(-10.0);
  create_channel_request.set_max_val(10.0);
  create_channel_request.set_units(VoltageUnits2::VOLTAGE_UNITS2_VOLTS);
  auto create_channel_response = CreateAIVoltageChanResponse{};
  raise_if_error(
    client.CreateAIVoltageChan(&create_channel_context, create_channel_request, &create_channel_response),
    create_channel_context);

  // ::grpc::ClientContext cfg_clk_context;
  // auto cfg_clk_request = CfgSampClkTimingRequest{};
  // cfg_clk_request.mutable_task()->CopyFrom(daqmx_read_task);
  // cfg_clk_request.set_rate(100.0);
  // cfg_clk_request.set_active_edge(Edge1::EDGE1_RISING);
  // cfg_clk_request.set_sample_mode(AcquisitionType::ACQUISITION_TYPE_HW_TIMED_SINGLE_POINT);
  // cfg_clk_request.set_samps_per_chan(10);
  // auto cfg_clk_response = CfgSampClkTimingResponse{};
  // raise_if_error(
  //   client.CfgSampClkTiming(&cfg_clk_context, cfg_clk_request, &cfg_clk_response),
  //   cfg_clk_context);

  // ::grpc::ClientContext set_read_attribute_context;
  // auto set_read_attribute_request = SetReadAttributeInt32Request{};
  // set_read_attribute_request.mutable_task()->CopyFrom(daqmx_read_task);
  // set_read_attribute_request.set_attribute(ReadInt32Attribute::READ_ATTRIBUTE_WAIT_MODE);
  // set_read_attribute_request.set_value(ReadInt32AttributeValues::READ_INT32_WAIT_MODE_POLL);
  // auto set_read_attribute_response = SetReadAttributeInt32Response{};
  // raise_if_error(
  //   client.SetReadAttributeInt32(&set_read_attribute_context, set_read_attribute_request, &set_read_attribute_response),
  //   set_read_attribute_context);

  ::grpc::ClientContext start_task_context;
  StartTaskRequest start_task_request;
  start_task_request.mutable_task()->CopyFrom(daqmx_read_task);
  StartTaskResponse start_task_response;
  raise_if_error(
    client.StartTask(&start_task_context, start_task_request, &start_task_response),
    start_task_context);

  return daqmx_read_task;
}
::nidevice_grpc::Session create_and_configure_write_task(NiDAQmx::Stub &client, const std::string &PHYSICAL_CHANNEL_WRITE)
{
    ::grpc::ClientContext create_task_context;
    auto create_task_request = CreateTaskRequest{};
    create_task_request.set_session_name("my write task");
    auto create_task_response = CreateTaskResponse{};
    raise_if_error(
        client.CreateTask(&create_task_context, create_task_request, &create_task_response),
        create_task_context);
    auto daqmx_write_task = create_task_response.task();

    ::grpc::ClientContext create_channel_context;
    auto create_channel_request = CreateAOVoltageChanRequest{};
    create_channel_request.mutable_task()->CopyFrom(daqmx_write_task);
    create_channel_request.set_physical_channel(PHYSICAL_CHANNEL_WRITE);
    create_channel_request.set_min_val(-10.0);
    create_channel_request.set_max_val(10.0);
    create_channel_request.set_units(VoltageUnits2::VOLTAGE_UNITS2_VOLTS);
    auto create_channel_response = CreateAOVoltageChanResponse{};
    raise_if_error(
        client.CreateAOVoltageChan(&create_channel_context, create_channel_request, &create_channel_response),
        create_channel_context);

    // ::grpc::ClientContext cfg_clk_context;
    // auto cfg_clk_request = CfgSampClkTimingRequest{};
    // cfg_clk_request.mutable_task()->CopyFrom(daqmx_write_task);
    // cfg_clk_request.set_rate(100.0);
    // cfg_clk_request.set_active_edge(Edge1::EDGE1_RISING);
    // cfg_clk_request.set_sample_mode(AcquisitionType::ACQUISITION_TYPE_HW_TIMED_SINGLE_POINT);
    // cfg_clk_request.set_samps_per_chan(10);
    // auto cfg_clk_response = CfgSampClkTimingResponse{};
    // raise_if_error(
    //     client.CfgSampClkTiming(&cfg_clk_context, cfg_clk_request, &cfg_clk_response),
    //     cfg_clk_context);

    // ::grpc::ClientContext set_write_attribute_context;
    // auto set_write_attribute_request = SetWriteAttributeInt32Request{};
    // set_write_attribute_request.mutable_task()->CopyFrom(daqmx_write_task);
    // set_write_attribute_request.set_attribute(WriteInt32Attribute::WRITE_ATTRIBUTE_WAIT_MODE);
    // set_write_attribute_request.set_value(WriteInt32AttributeValues::WRITE_INT32_WAIT_MODE_POLL);
    // auto set_write_attribute_response = SetWriteAttributeInt32Response{};
    // raise_if_error(
    //     client.SetWriteAttributeInt32(&set_write_attribute_context, set_write_attribute_request, &set_write_attribute_response),
    //     set_write_attribute_context);

    ::grpc::ClientContext start_task_context;
    StartTaskRequest start_task_request;
    start_task_request.mutable_task()->CopyFrom(daqmx_write_task);
    StartTaskResponse start_task_response;
    raise_if_error(
        client.StartTask(&start_task_context, start_task_request, &start_task_response),
        start_task_context);

    return daqmx_write_task;
}

int main(int argc, char **argv)
{
  if (argc >= 2) {
    SERVER_ADDRESS = argv[1];
  }
  if (argc >= 3) {
    SERVER_PORT = argv[2];
  }
  if (argc >= 4) {
    READ_DEVICE = argv[3];
  }
  if (argc >= 5) {
    WRITE_DEVICE = argv[4];
  }
  if (argc >= 6) {
    NUM_CHANNELS = std::stoi(argv[5]);
  }

  if (NUM_CHANNELS == 1) {
    PHYSICAL_CHANNEL_READ = READ_DEVICE + "/ai0";
    PHYSICAL_CHANNEL_WRITE = WRITE_DEVICE + "/ao0";
  } else {
      PHYSICAL_CHANNEL_READ = READ_DEVICE + "/ai0:" + std::to_string(NUM_CHANNELS - 1);
      PHYSICAL_CHANNEL_WRITE = WRITE_DEVICE + "/ao0:" + std::to_string(NUM_CHANNELS - 1);
  }
  auto target_str = SERVER_ADDRESS + ":" + SERVER_PORT;

  std::cout << "Configuration:\n";
  std::cout << "  Server: " << target_str << "\n";
  std::cout << "  Number of channels: " << NUM_CHANNELS << "\n";
  std::cout << "  Read channel: " << PHYSICAL_CHANNEL_READ << "\n";
  std::cout << "  Write channel: " << PHYSICAL_CHANNEL_WRITE << "\n";

  auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
  NiDAQmx::Stub client(channel);
  ni::data_monikers::DataMoniker::Stub moniker_service(channel);

  try {

   
    std::vector<double> write_data_float64(NUM_CHANNELS, 1.0);
    std::cout << "Set up Read" << std::endl;
    auto daqmx_read_task = create_and_configure_read_task(client, PHYSICAL_CHANNEL_READ);

    std::cout << "Set up Write" << std::endl;
    auto daqmx_write_task = create_and_configure_write_task(client, PHYSICAL_CHANNEL_WRITE);

    // Setup the read moniker
    std::cout << "Set up Read Moniker" << std::endl;
    ::grpc::ClientContext begin_read_context;
    auto begin_read_request = BeginReadAnalogF64Request{};
    begin_read_request.mutable_task()->CopyFrom(daqmx_read_task);
    begin_read_request.set_num_samps_per_chan(1);
    begin_read_request.set_timeout(10.0);
    begin_read_request.set_array_size_in_samps(1*NUM_CHANNELS);
    begin_read_request.set_fill_mode(GroupBy::GROUP_BY_GROUP_BY_CHANNEL);
        auto begin_read_response = BeginReadAnalogF64Response{};
    raise_if_error(
      client.BeginReadAnalogF64(&begin_read_context, begin_read_request, &begin_read_response),
      begin_read_context);


    auto daqmx_read_moniker = new ni::data_monikers::Moniker(begin_read_response.moniker());
    
    // ::grpc::ClientContext stream_read_context;
    // ni::data_monikers::MonikerList read_request;
    // read_request.mutable_read_monikers()->AddAllocated(daqmx_read_moniker);
    // auto read_stream = moniker_service.StreamRead(&stream_read_context, read_request);


    // Setup the write moniker
    std::cout << "Setup Write Moniker" << std::endl;
    ::grpc::ClientContext begin_write_context;
  auto begin_write_request = BeginWriteAnalogF64Request{};
  begin_write_request.mutable_task()->CopyFrom(daqmx_write_task);
  begin_write_request.set_num_samps_per_chan(1);
  begin_write_request.set_timeout(10.0);
  begin_write_request.set_data_layout(GroupBy::GROUP_BY_GROUP_BY_CHANNEL);
    auto begin_write_response = BeginWriteAnalogF64Response{};
    raise_if_error(
      client.BeginWriteAnalogF64(&begin_write_context, begin_write_request, &begin_write_response),
      begin_write_context);

    
    auto daqmx_write_moniker = new ni::data_monikers::Moniker(begin_write_response.moniker());

    //Setup Read Write Stream
    ::grpc::ClientContext stream_read_write_context;
    ni::data_monikers::MonikerList read_write_request;

    
    
    
    // read_write_request.mutable_read_monikers()->AddAllocated(daqmx_read_moniker);
    // read_write_request.mutable_write_monikers()->AddAllocated(daqmx_write_moniker);
    // std::cout << "Start Read/Write Stream" << std::endl;  
    // auto read_write_stream = moniker_service.StreamReadWrite(&stream_read_write_context);
        

    grpc::ClientContext moniker_context;
    ni::data_monikers::BeginMonikerSidebandStreamRequest sideband_request;
    ni::data_monikers::BeginMonikerSidebandStreamResponse sideband_response;
    sideband_request.set_strategy(ni::data_monikers::SidebandStrategy::SOCKETS_LOW_LATENCY);
    sideband_request.mutable_monikers()->mutable_read_monikers()->AddAllocated(daqmx_read_moniker);
    sideband_request.mutable_monikers()->mutable_write_monikers()->AddAllocated(daqmx_write_moniker);
    auto write_stream = moniker_service.BeginSidebandStream(&moniker_context, sideband_request, &sideband_response);
    if(!write_stream.ok()) {
       std::cout << "ERROR BeginSidebandStream: (" << write_stream.error_code() << ") " << write_stream.error_message() << std::endl;
    }
    auto sideband_token = InitClientSidebandData(sideband_response);
    std::cout << "InitClientSidebandData complete with token " << sideband_token << std::endl;
    

    // Read data and write data
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      ni::data_monikers::MonikerReadResponse read_data_result;
      nidaqmx_grpc::MonikerWriteAnalogF64Request write_values_array_f64;
      ni::data_monikers::SidebandWriteRequest sideband_request;
      
      
      write_values_array_f64.mutable_write_array()->Add(write_data_float64.begin(), write_data_float64.end());
      sideband_request.mutable_values()->add_values()->PackFrom(write_values_array_f64);

      WriteSidebandMessage(sideband_token, sideband_request);
       std::cout << "Write Sideband Message done" << std::endl;

      MonikerReadAnalogF64Response read_analog_f64_response;
      ni::data_monikers::SidebandReadResponse read_result;
      ReadSidebandMessage(sideband_token, &read_result);
      auto status = read_result.values().values(0).UnpackTo(&read_analog_f64_response);
   
        std::cout << "Status of Unpack" << status << std::endl;
  
      std::cout << "Read data..." << std::endl;
      print_array(read_analog_f64_response);
      
      }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "\nLoop execution time: " << duration.count() << " ms" << std::endl;
    std::cout << "Average time per iteration: " << (duration.count() / static_cast<double>(NUM_ITERATIONS)) << " ms" << std::endl;

    ni::data_monikers::SidebandWriteRequest cancel_request;
    cancel_request.set_cancel(true);
    WriteSidebandMessage(sideband_token, cancel_request);
    CloseSidebandData(sideband_token);
    auto request_read = BeginReadAnalogF64Request{};

    std::cout << "Cleaning up." << std::endl;

    ::grpc::ClientContext stop_task_context;
    StopTaskRequest stop_task_request;
    stop_task_request.mutable_task()->CopyFrom(daqmx_read_task);
    StopTaskResponse stop_task_response;
    raise_if_error(
      client.StopTask(&stop_task_context, stop_task_request, &stop_task_response),
      stop_task_context);

    ::grpc::ClientContext clear_task_context;
    ClearTaskRequest clear_task_request;
    clear_task_request.mutable_task()->CopyFrom(daqmx_read_task);
    ClearTaskResponse clear_task_response;
    raise_if_error(
      client.ClearTask(&clear_task_context, clear_task_request, &clear_task_response),
      clear_task_context);


    ::grpc::ClientContext stop_write_task_context;
    StopTaskRequest stop_write_task_request;
    stop_write_task_request.mutable_task()->CopyFrom(daqmx_write_task);
    StopTaskResponse stop_write_task_response;
    raise_if_error(
      client.StopTask(&stop_write_task_context, stop_write_task_request, &stop_write_task_response),
      stop_write_task_context);

    ::grpc::ClientContext clear_write_task_context;
    ClearTaskRequest clear_write_task_request;
    clear_write_task_request.mutable_task()->CopyFrom(daqmx_write_task);
    ClearTaskResponse clear_write_task_response;
    raise_if_error(
      client.ClearTask(&clear_write_task_context, clear_write_task_request, &clear_write_task_response),
      clear_write_task_context);
  }
  catch (const grpc_driver_error& e) {
    std::string error_message = e.what();

    for (const auto& entry : e.Trailers()) {
      if (entry.first == "ni-error") {
        error_message += "\nError status: " + entry.second;
      }
    }

    if (e.StatusCode() == grpc::StatusCode::UNAVAILABLE) {
      error_message = "Failed to connect to server on " + SERVER_ADDRESS + ":" + SERVER_PORT;
    }
    else if (e.StatusCode() == grpc::StatusCode::UNIMPLEMENTED) {
      error_message = "The operation is not implemented or is not supported/enabled in this service";
    }

    std::cout << "Exception: " << error_message << std::endl;
  }
}