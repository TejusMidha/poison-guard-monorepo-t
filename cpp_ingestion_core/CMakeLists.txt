cmake_minimum_required(VERSION 3.18)
project(AI_PoisonGuard_Core)
cmake_minimum_required(VERSION 3.18)
project(AI_PoisonGuard_Core)

set(CMAKE_CXX_STANDARD 17)

# 1. LibTorch Config
set(CMAKE_PREFIX_PATH "/opt/libtorch") # Update to your path
find_package(Torch REQUIRED)

# 2. ZeroMQ Config
find_package(ZeroMQ REQUIRED)

find_package(OpenSSL REQUIRED)

# 3. CUDA Config (For your RTX 4070)
set(CMAKE_CUDA_COMPILER /opt/cuda/bin/nvcc)
enable_language(CUDA)

include_directories(${TORCH_INCLUDE_DIRS} ${ZeroMQ_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} /usr/include)

add_executable(ingestion_core 
    src/main.cpp 
    src/zmq_client.cpp
)

# Link both Torch and the ZeroMQ library
target_link_libraries(ingestion_core "${TORCH_LIBRARIES}" zmq OpenSSL::Crypto)
