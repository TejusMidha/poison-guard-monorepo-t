#ifndef ZMQ_CLIENT_HPP
#define ZMQ_CLIENT_HPP

#include <string>
#include <zmq.h>
#include <zmq.hpp>

class ZMQClient {
public:
    ZMQClient(const std::string& endpoint);
    void send_vector_telemetry(std::string batch_id, float feat1, float feat2, float label, std::string profile);
    void send_telemetry(int flagged, float mean, float std);

private:
    zmq::context_t context; 
    zmq::socket_t socket;
};

#endif
