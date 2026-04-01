#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/sha.h> // Ensure 'openssl' is installed
#include <torch/torch.h>
#include <iomanip>
#include "zmq_client.hpp"

/**
 * Layer 1: Statistical Anomaly Detection
 * Formula: $$z = \frac{x - \mu}{\sigma}$$
 */

std::string calculate_sha256(void* addr, size_t size) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)addr, size, hash);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

int main() {
  const char* filepath = "../shared_data/shared_data/clean_upi.csv";
    const std::string EXPECTED_HASH = "6e05f8fbff2d7642e9c0cd442a992d6316b0222714cc24a5e24f2a7d943327fd"; // From sha256sum

    // 1. Zero-Copy mmap Ingestion 
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) { std::cerr << "File Error\n"; return 1; }

    struct stat sb;
    fstat(fd, &sb);
    void* addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    // 2. Blockchain Provenance Check 
    std::string live_hash = calculate_sha256(addr, sb.st_size);
    if (live_hash != EXPECTED_HASH) {
        std::cerr << "!!! PROVENANCE BREACH: Hash Mismatch !!!\n";
        // Future: Log to ProvenanceGatekeeper.sol [cite: 603]
    }

    // 3. LibTorch Layer 1 Anomaly Detection [cite: 607]
    // Note: In MVP, we skip CSV headers and map raw floats
    auto options = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);
    
    // We treat the data as a 1D tensor of 'amount' values for this layer
    auto amount_tensor = torch::from_blob(addr, {(long)(sb.st_size / sizeof(float))}, options);

    float mean = amount_tensor.mean().item<float>();
    float std_dev = amount_tensor.std().item<float>();

    // Calculate Z-scores to find statistical outliers [cite: 427]
    auto z_scores = (amount_tensor - mean) / (std_dev + 1e-5);
    auto outliers = z_scores.abs() > 3.0;
    int flagged_count = outliers.sum().item<int>();

    // 4. ZeroMQ IPC Stream to Python [cite: 608]
    ZMQClient dw_client("tcp://localhost:5555");
    dw_client.send_telemetry(flagged_count, mean, std_dev);

    std::cout << "[C++ CORE] Layer 1 Complete. Flagged: " << flagged_count << " anomalies.\n";

    munmap(addr, sb.st_size);
    close(fd);
    return 0;
}
