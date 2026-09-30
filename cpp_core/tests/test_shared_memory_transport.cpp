#include "../transport/shared_memory_transport.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    constexpr const char* kName =
        "/ugv_shared_memory_test";

    constexpr std::size_t kFrameSize =
        1024;

    std::vector<std::uint8_t> write_data(
        kFrameSize);

    std::vector<std::uint8_t> read_data(
        kFrameSize);

    for (std::size_t i = 0;
         i < kFrameSize;
         ++i) {

        write_data[i] =
            static_cast<std::uint8_t>(
                i % 256);
    }

    ugv::SharedMemoryTransport producer;
    ugv::SharedMemoryTransport consumer;

    assert(
        producer.create(
            kName,
            kFrameSize));

    assert(
        consumer.open(
            kName,
            kFrameSize));

    assert(
        producer.publish(
            write_data.data(),
            write_data.size(),
            1));

    std::uint64_t sequence = 0;

    assert(
        consumer.consume(
            read_data.data(),
            read_data.size(),
            sequence));

    assert(sequence == 1);
    assert(read_data == write_data);

    producer.close();
    consumer.close();

    return 0;
}