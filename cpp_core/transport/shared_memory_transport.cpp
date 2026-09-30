#include "shared_memory_transport.hpp"

#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ugv {

namespace {

bool acquire_lock(std::uint32_t* lock) noexcept {
    std::uint32_t expected = 0;

    return __atomic_compare_exchange_n(
        lock,
        &expected,
        1,
        false,
        __ATOMIC_ACQUIRE,
        __ATOMIC_RELAXED);
}

void release_lock(std::uint32_t* lock) noexcept {
    __atomic_store_n(
        lock,
        0,
        __ATOMIC_RELEASE);
}

}  // namespace

SharedMemoryTransport::~SharedMemoryTransport() {
    close();
}

bool SharedMemoryTransport::create(
    const std::string& name,
    std::size_t frame_size) {

    close();

    if (name.empty() || frame_size == 0) {
        return false;
    }

    const std::size_t total_size =
        kHeaderSize +
        (kBufferCount * frame_size);

    const int fd = ::shm_open(
        name.c_str(),
        O_CREAT | O_RDWR,
        0600);

    if (fd == -1) {
        return false;
    }

    if (::ftruncate(
            fd,
            static_cast<off_t>(total_size)) == -1) {

        ::close(fd);
        ::shm_unlink(name.c_str());
        return false;
    }

    void* mapped = ::mmap(
        nullptr,
        total_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    if (mapped == MAP_FAILED) {
        ::close(fd);
        ::shm_unlink(name.c_str());
        return false;
    }

    file_descriptor_ = fd;
    memory_ = mapped;
    frame_size_ = frame_size;
    mapped_size_ = total_size;
    owner_ = true;
    name_ = name;

    std::memset(
        memory_,
        0,
        mapped_size_);

    SharedHeader* shared = header();

    shared->magic = kMagic;
    shared->version = kVersion;
    shared->frame_size =
        static_cast<std::uint32_t>(frame_size);
    shared->buffer_count =
        static_cast<std::uint32_t>(kBufferCount);

    shared->published_sequence = 0;
    shared->published_buffer = 0;

    shared->producer_lock = 0;
    shared->consumer_lock = 0;

    return true;
}

bool SharedMemoryTransport::open(
    const std::string& name,
    std::size_t frame_size) {

    close();

    if (name.empty() || frame_size == 0) {
        return false;
    }

    const int fd = ::shm_open(
        name.c_str(),
        O_RDWR,
        0600);

    if (fd == -1) {
        return false;
    }

    struct stat status {};

    if (::fstat(fd, &status) == -1) {
        ::close(fd);
        return false;
    }

    const std::size_t expected_size =
        kHeaderSize +
        (kBufferCount * frame_size);

    if (status.st_size <
        static_cast<off_t>(expected_size)) {

        ::close(fd);
        return false;
    }

    void* mapped = ::mmap(
        nullptr,
        expected_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    if (mapped == MAP_FAILED) {
        ::close(fd);
        return false;
    }

    file_descriptor_ = fd;
    memory_ = mapped;
    frame_size_ = frame_size;
    mapped_size_ = expected_size;
    owner_ = false;
    name_ = name;

    const SharedHeader* shared = header();

    if (shared->magic != kMagic ||
        shared->version != kVersion ||
        shared->frame_size != frame_size ||
        shared->buffer_count != kBufferCount) {

        close();
        return false;
    }

    return true;
}

bool SharedMemoryTransport::publish(
    const std::uint8_t* data,
    std::size_t size,
    std::uint64_t sequence) {

    if (!isOpen() ||
        data == nullptr ||
        size != frame_size_ ||
        sequence == 0) {

        return false;
    }

    SharedHeader* shared = header();

    if (!acquire_lock(&shared->producer_lock)) {
        return false;
    }

    const std::size_t current =
        shared->published_buffer;

    const std::size_t next =
        (current + 1) % kBufferCount;

    std::uint8_t* destination =
        buffer(next);

    std::memcpy(
        destination,
        data,
        frame_size_);

    __atomic_thread_fence(
        __ATOMIC_RELEASE);

    shared->published_buffer =
        static_cast<std::uint32_t>(next);

    shared->published_sequence =
        sequence;

    release_lock(
        &shared->producer_lock);

    return true;
}

bool SharedMemoryTransport::consume(
    std::uint8_t* data,
    std::size_t size,
    std::uint64_t& sequence) {

    if (!isOpen() ||
        data == nullptr ||
        size != frame_size_) {

        return false;
    }

    SharedHeader* shared = header();

    if (!acquire_lock(&shared->consumer_lock)) {
        return false;
    }

    __atomic_thread_fence(
        __ATOMIC_ACQUIRE);

    const std::size_t index =
        shared->published_buffer;

    const std::uint64_t published_sequence =
        shared->published_sequence;

    if (published_sequence == 0 ||
        index >= kBufferCount) {

        release_lock(
            &shared->consumer_lock);

        return false;
    }

    const std::uint8_t* source =
        buffer(index);

    std::memcpy(
        data,
        source,
        frame_size_);

    sequence = published_sequence;

    release_lock(
        &shared->consumer_lock);

    return true;
}

void SharedMemoryTransport::close() noexcept {

    if (memory_ != nullptr) {
        ::munmap(
            memory_,
            mapped_size_);
    }

    if (file_descriptor_ != -1) {
        ::close(
            file_descriptor_);
    }

    if (owner_ && !name_.empty()) {
        ::shm_unlink(
            name_.c_str());
    }

    memory_ = nullptr;
    file_descriptor_ = -1;
    frame_size_ = 0;
    mapped_size_ = 0;
    owner_ = false;
    name_.clear();
}

bool SharedMemoryTransport::isOpen() const noexcept {
    return (
        memory_ != nullptr &&
        file_descriptor_ != -1 &&
        frame_size_ > 0
    );
}

std::size_t
SharedMemoryTransport::frameSize() const noexcept {
    return frame_size_;
}

SharedMemoryTransport::SharedHeader*
SharedMemoryTransport::header() noexcept {
    return static_cast<SharedHeader*>(
        memory_);
}

const SharedMemoryTransport::SharedHeader*
SharedMemoryTransport::header() const noexcept {
    return static_cast<const SharedHeader*>(
        memory_);
}

std::uint8_t*
SharedMemoryTransport::buffer(
    std::size_t index) noexcept {

    auto* base =
        static_cast<std::uint8_t*>(
            memory_);

    return base +
           kHeaderSize +
           (index * frame_size_);
}

const std::uint8_t*
SharedMemoryTransport::buffer(
    std::size_t index) const noexcept {

    const auto* base =
        static_cast<const std::uint8_t*>(
            memory_);

    return base +
           kHeaderSize +
           (index * frame_size_);
}

}  // namespace ugv