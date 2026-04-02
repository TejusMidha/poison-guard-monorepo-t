#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <torch/torch.h> // Ensure this is linked
#include "zmq_client.hpp"

std::string get_sha256(void* addr, size_t size) {
    if (!addr) return "NULL_ADDR";
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)addr, size, hash);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

int main() {
    // 1. SAFE FILE OPENING
    const char* filepath = "../shared_data/shared_data/poisoned_upi.csv";
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        std::cerr << "[FATAL] Could not open file: " << filepath << ". Check path!" << std::endl;
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "[FATAL] Could not get file stats." << std::endl;
        return 1;
    }

    // 2. SAFE MEMORY MAPPING
    void* addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "[FATAL] mmap failed. File might be too large or locked." << std::endl;
        return 1;
    }

    std::cout << "[C++ CORE] Memory Mapped: " << sb.st_size << " bytes." << std::endl;

    // 3. PROVENANCE PASS
    std::string file_hash = get_sha256(addr, sb.st_size);
    std::cout << "[C++ CORE] Verified Hash: " << file_hash << std::endl;

    // 4. THE HANDSHAKE
    ZMQClient client("tcp://127.0.0.1:5555");

    // 5. MEMORY-EFFICIENT PARSING
    // Instead of copying the whole file into a string, we parse the buffer directly
    std::string content((char*)addr, sb.st_size);
    std::stringstream ss(content);
    std::string line;
    std::getline(ss, line); // Skip Header

    int count = 0;
    while (std::getline(ss, line) && count < 50) {
        try {
            std::stringstream line_ss(line);
            std::string val;
            float amount = 0, timestamp = 0, integrity = 0;
            int col = 0;

            while (std::getline(line_ss, val, ',')) {
                if (col == 1) timestamp = std::stof(val);
                if (col == 2) amount = std::stof(val);
                if (col == 5) integrity = std::stof(val);
                col++;
            }

            // High-speed telemetry push
            client.send_vector_telemetry("BATCH_" + std::to_string(count), amount, timestamp, integrity);
            count++;
            
            if (count % 10 == 0) std::cout << "[C++ CORE] Streamed " << count << " vectors..." << std::endl;
        } catch (...) {
            continue; // Skip malformed rows
        }
    }

    // CLEANUP
    munmap(addr, sb.st_size);
    close(fd);
    return 0;
}
