#include "../transport/python_perception_reader.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr const char* kSharedMemoryName =
    "/ugv_perception_frame";

constexpr std::size_t kFrameSize =
    ugv::PythonPerceptionReader::kFrameSize;

}  // namespace

int main() {

    const int fd = ::shm_open(
        kSharedMemoryName,
        O_RDONLY,
        0600);

    assert(fd != -1);

    struct stat status {};

    assert(::fstat(fd, &status) == 0);

    assert(
        static_cast<std::size_t>(status.st_size)
        >= kFrameSize);

    void* mapped = ::mmap(
        nullptr,
        kFrameSize,
        PROT_READ,
        MAP_SHARED,
        fd,
        0);

    assert(mapped != MAP_FAILED);

    ugv::PerceptionFrame frame;

    ugv::PythonPerceptionReader reader;

    const bool decoded =
        reader.decode(
            static_cast<const std::uint8_t*>(mapped),
            kFrameSize,
            frame);

    assert(decoded);
    assert(frame.isValid());

    assert(frame.width == 512);
    assert(frame.height == 512);
    assert(frame.timestamp_ns != 0);

    assert(
        frame.segmentation.size()
        == 512ULL * 512ULL);

    assert(
        frame.confidence.size()
        == 512ULL * 512ULL);

    assert(
        frame.depth.size()
        == 512ULL * 512ULL);

    assert(
        frame.roughness.size()
        == 512ULL * 512ULL);

    std::cout
        << "PYTHON PERCEPTION DECODE: PASS\n"
        << "Sequence-independent frame decoded\n"
        << "Dimensions: "
        << frame.width
        << " x "
        << frame.height
        << "\n";

    ::munmap(mapped, kFrameSize);
    ::close(fd);

    return 0;
}