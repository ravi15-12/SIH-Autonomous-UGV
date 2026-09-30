#pragma once

#include "python_perception_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ugv {

class SynchronizedPerceptionTransport {
public:
    static constexpr std::size_t kBufferCount = 1;

    SynchronizedPerceptionTransport() = default;

    ~SynchronizedPerceptionTransport();

    SynchronizedPerceptionTransport(
        const SynchronizedPerceptionTransport&) = delete;

    SynchronizedPerceptionTransport& operator=(
        const SynchronizedPerceptionTransport&) = delete;

    bool open(
        const std::string& shared_memory_name =
            "ugv_perception_frame",
        const std::string& ready_semaphore_name =
            "ugv_perception_ready",
        const std::string& free_semaphore_name =
            "ugv_perception_free");

    bool consume(
        PerceptionFrame& frame);

    void close() noexcept;

    bool isOpen() const noexcept;

private:
    int file_descriptor_ = -1;

    void* memory_ = nullptr;

    std::size_t mapped_size_ = 0;

    void* ready_semaphore_ = nullptr;
    void* free_semaphore_ = nullptr;

    PythonPerceptionReader reader_;

    std::string shared_memory_name_;
    std::string ready_semaphore_name_;
    std::string free_semaphore_name_;
};

}  // namespace ugv