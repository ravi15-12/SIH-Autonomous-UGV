#include "synchronized_pose_transport.hpp"

#include "pose_frame_header.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <semaphore.h>

namespace ugv {

SynchronizedPoseTransport::~SynchronizedPoseTransport() {
    close();
}

bool SynchronizedPoseTransport::open() {
    if (mapped_memory_ != nullptr ||
        ready_semaphore_ != nullptr ||
        free_semaphore_ != nullptr) {
        return false;
    }

    shm_fd_ = ::shm_open(
        kSharedMemoryName,
        O_RDWR,
        0600
    );

    if (shm_fd_ < 0) {
        return false;
    }

    struct stat status {};

    if (::fstat(shm_fd_, &status) != 0) {
        close();
        return false;
    }

    if (status.st_size < static_cast<off_t>(
            sizeof(PoseFrameHeader))) {
        close();
        return false;
    }

    mapped_size_ =
        static_cast<std::size_t>(status.st_size);

    mapped_memory_ = ::mmap(
        nullptr,
        mapped_size_,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd_,
        0
    );

    if (mapped_memory_ == MAP_FAILED) {
        mapped_memory_ = nullptr;
        close();
        return false;
    }

    ready_semaphore_ = ::sem_open(
        kReadySemaphoreName,
        0
    );

    if (ready_semaphore_ == SEM_FAILED) {
        ready_semaphore_ = nullptr;
        close();
        return false;
    }

    free_semaphore_ = ::sem_open(
        kFreeSemaphoreName,
        0
    );

    if (free_semaphore_ == SEM_FAILED) {
        free_semaphore_ = nullptr;
        close();
        return false;
    }

    return true;
}

bool SynchronizedPoseTransport::consume(
    PoseFrame& output
) {
    output = {};

    if (mapped_memory_ == nullptr ||
        ready_semaphore_ == nullptr ||
        free_semaphore_ == nullptr) {
        return false;
    }

    int result;

    do {
        result = ::sem_wait(
            static_cast<sem_t*>(ready_semaphore_)
        );
    } while (result != 0 && errno == EINTR);

    if (result != 0) {
        return false;
    }

    const bool decoded = reader_.decode(
        static_cast<const std::uint8_t*>(
            mapped_memory_
        ),
        mapped_size_,
        output
    );

    ::sem_post(
        static_cast<sem_t*>(free_semaphore_)
    );

    return decoded;
}

void SynchronizedPoseTransport::close() {
    if (ready_semaphore_ != nullptr) {
        ::sem_close(
            static_cast<sem_t*>(ready_semaphore_)
        );
        ready_semaphore_ = nullptr;
    }

    if (free_semaphore_ != nullptr) {
        ::sem_close(
            static_cast<sem_t*>(free_semaphore_)
        );
        free_semaphore_ = nullptr;
    }

    if (mapped_memory_ != nullptr) {
        ::munmap(
            mapped_memory_,
            mapped_size_
        );
        mapped_memory_ = nullptr;
    }

    mapped_size_ = 0;

    if (shm_fd_ >= 0) {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
}

}  // namespace ugv