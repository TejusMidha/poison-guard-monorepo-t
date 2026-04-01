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

void ZMQClient::send_telemetry(int flagged, float mean, float std) {
    // Lean JSON payload for the Python FastAPI backend [cite: 618]
    std::string json_msg = "{\"flagged\":" + std::to_string(flagged) + 
                           ",\"mean\":" + std::to_string(mean) + 
                           ",\"std\":" + std::to_string(std) + "}";
    
    socket.send(zmq::buffer(json_msg), zmq::send_flags::none);
}

void ZMQClient::send_alert(const std::string& message) {
    std::string json_msg = "{\"type\":\"ALERT\",\"message\":\"" + message + "\"}";
    socket.send(zmq::buffer(json_msg), zmq::send_flags::none);
}
