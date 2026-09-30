#include "synchronized_perception_transport.hpp"

#include <cerrno>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ugv {

namespace {

constexpr std::size_t kFrameSize =
    PythonPerceptionReader::kFrameSize;

}  // namespace

SynchronizedPerceptionTransport::~SynchronizedPerceptionTransport() {
    close();
}

bool SynchronizedPerceptionTransport::open(
    const std::string& shared_memory_name,
    const std::string& ready_semaphore_name,
    const std::string& free_semaphore_name) {

    close();

    if (shared_memory_name.empty() ||
        ready_semaphore_name.empty() ||
        free_semaphore_name.empty()) {
        return false;
    }

    const std::string ready_name =
        "/" + ready_semaphore_name.substr(
            ready_semaphore_name.find_first_not_of('/'));

    const std::string free_name =
        "/" + free_semaphore_name.substr(
            free_semaphore_name.find_first_not_of('/'));

    const std::string shm_name =
    "/" + shared_memory_name.substr(
        shared_memory_name.find_first_not_of('/'));

    const int fd = ::shm_open(
    shm_name.c_str(),
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

    if (static_cast<std::size_t>(status.st_size) <
        kFrameSize) {
        ::close(fd);
        return false;
    }

    void* mapped = ::mmap(
        nullptr,
        kFrameSize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    if (mapped == MAP_FAILED) {
        ::close(fd);
        return false;
    }

    sem_t* ready = ::sem_open(
        ready_name.c_str(),
        0);

    if (ready == SEM_FAILED) {
        ::munmap(mapped, kFrameSize);
        ::close(fd);
        return false;
    }

    sem_t* free = ::sem_open(
        free_name.c_str(),
        0);

    if (free == SEM_FAILED) {
        ::sem_close(ready);
        ::munmap(mapped, kFrameSize);
        ::close(fd);
        return false;
    }

    file_descriptor_ = fd;
    memory_ = mapped;
    mapped_size_ = kFrameSize;

    ready_semaphore_ =
        static_cast<void*>(ready);

    free_semaphore_ =
        static_cast<void*>(free);

    shared_memory_name_ =
        shared_memory_name;

    ready_semaphore_name_ =
        ready_name;

    free_semaphore_name_ =
        free_name;

    return true;
}

bool SynchronizedPerceptionTransport::consume(
    PerceptionFrame& frame) {

    if (!isOpen()) {
        return false;
    }

    sem_t* ready =
        static_cast<sem_t*>(ready_semaphore_);

    sem_t* free =
        static_cast<sem_t*>(free_semaphore_);

    while (true) {
        if (::sem_wait(ready) == 0) {
            break;
        }

        if (errno != EINTR) {
            return false;
        }
    }

    const bool decoded =
        reader_.decode(
            static_cast<const std::uint8_t*>(
                memory_),
            kFrameSize,
            frame);

    if (::sem_post(free) != 0) {
        return false;
    }

    return decoded;
}

void SynchronizedPerceptionTransport::close() noexcept {

    if (ready_semaphore_ != nullptr) {
        ::sem_close(
            static_cast<sem_t*>(
                ready_semaphore_));
    }

    if (free_semaphore_ != nullptr) {
        ::sem_close(
            static_cast<sem_t*>(
                free_semaphore_));
    }

    if (memory_ != nullptr) {
        ::munmap(
            memory_,
            mapped_size_);
    }

    if (file_descriptor_ != -1) {
        ::close(
            file_descriptor_);
    }

    ready_semaphore_ = nullptr;
    free_semaphore_ = nullptr;
    memory_ = nullptr;
    mapped_size_ = 0;
    file_descriptor_ = -1;

    shared_memory_name_.clear();
    ready_semaphore_name_.clear();
    free_semaphore_name_.clear();
}

bool SynchronizedPerceptionTransport::isOpen()
    const noexcept {

    return (
        memory_ != nullptr &&
        file_descriptor_ != -1 &&
        ready_semaphore_ != nullptr &&
        free_semaphore_ != nullptr
    );
}

}  // namespace ugv