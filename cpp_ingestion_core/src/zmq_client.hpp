#ifndef ZMQ_CLIENT_HPP
#define ZMQ_CLIENT_HPP

#include <string>
#include <zmq.h>   // Low-level C headers
#include <zmq.hpp>

class ZMQClient {
public:
    ZMQClient(const std::string& endpoint);
    void send_telemetry(int flagged, float mean, float std);
    void send_alert(const std::string &message); // For hash mismatches

private:
    // These are the "fields" the compiler was complaining about
    zmq::context_t context; 
    zmq::socket_t socket;
};

#endif
#
