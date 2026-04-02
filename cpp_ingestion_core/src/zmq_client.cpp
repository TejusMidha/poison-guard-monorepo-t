#include "zmq_client.hpp"
#include <zmq.hpp>
#include <iostream>

ZMQClient::ZMQClient(const std::string &endpoint)
    : context(1), socket(context, zmq::socket_type::push)
{
  try {
        socket.connect(endpoint);
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ Connection Failed: " << e.what() << std::endl;
    }
}

void ZMQClient::send_vector_telemetry(std::string batch_id, float amount, float time, float integrity) {
    // Exact JSON schema for the Python Shadow Model
    std::string json_msg = "{"
        "\"batch_id\":\"" + batch_id + "\","
        "\"demo_vector\": [" + std::to_string(amount) + "," + std::to_string(time) + "," + std::to_string(integrity) + "],"
        "\"ingestion_rate\": \"1.4 GB/s\""
    "}";
    socket.send(zmq::buffer(json_msg), zmq::send_flags::none);
}

void ZMQClient::send_telemetry(int flagged, float mean, float std) {
    std::string json_msg = "{\"flagged\":" + std::to_string(flagged) + "}";
    socket.send(zmq::buffer(json_msg), zmq::send_flags::none);
}
