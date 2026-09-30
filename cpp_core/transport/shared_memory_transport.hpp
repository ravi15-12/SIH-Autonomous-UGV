#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ugv {

class SharedMemoryTransport {
public:
    static constexpr std::size_t kFrameSize = 3'407'908;
    static constexpr std::size_t kBufferCount = 2;

    SharedMemoryTransport() = default;
    ~SharedMemoryTransport();

    SharedMemoryTransport(const SharedMemoryTransport&) = delete;
    SharedMemoryTransport& operator=(const SharedMemoryTransport&) = delete;

    bool create(
        const std::string& name,
        std::size_t frame_size = kFrameSize);

    bool open(
        const std::string& name,
        std::size_t frame_size = kFrameSize);

    bool publish(
        const std::uint8_t* data,
        std::size_t size,
        std::uint64_t sequence);

    bool consume(
        std::uint8_t* data,
        std::size_t size,
        std::uint64_t& sequence);

    void close() noexcept;

    bool isOpen() const noexcept;

    std::size_t frameSize() const noexcept;

private:
    struct SharedHeader {
        std::uint32_t magic = 0;
        std::uint16_t version = 0;
        std::uint16_t reserved = 0;

        std::uint32_t frame_size = 0;
        std::uint32_t buffer_count = 0;

        std::uint64_t published_sequence = 0;
        std::uint32_t published_buffer = 0;
        std::uint32_t reserved2 = 0;

        std::uint32_t producer_lock = 0;
        std::uint32_t consumer_lock = 0;
    };

    static constexpr std::uint32_t kMagic = 0x55475653;
    static constexpr std::uint16_t kVersion = 2;

    static constexpr std::size_t kHeaderSize =
        sizeof(SharedHeader);

    int file_descriptor_ = -1;
    void* memory_ = nullptr;

    std::size_t frame_size_ = 0;
    std::size_t mapped_size_ = 0;

    bool owner_ = false;
    std::string name_;

    SharedHeader* header() noexcept;

    const SharedHeader* header() const noexcept;

    std::uint8_t* buffer(
        std::size_t index) noexcept;

    const std::uint8_t* buffer(
        std::size_t index) const noexcept;
};

}  // namespace ugv