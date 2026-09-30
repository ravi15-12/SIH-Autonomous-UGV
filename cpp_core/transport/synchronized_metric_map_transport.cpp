#include "synchronized_metric_map_transport.hpp"

#include <cerrno>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ugv {

SynchronizedMetricMapTransport::~SynchronizedMetricMapTransport() {
    close();
}

bool SynchronizedMetricMapTransport::open() {
    if (shm_fd_ != -1) {
        return true;
    }

    shm_fd_ = ::shm_open(
        kSharedMemoryName,
        O_RDWR,
        0666
    );

    if (shm_fd_ == -1) {
        return false;
    }

    struct stat st {};
    if (::fstat(shm_fd_, &st) != 0) {
        close();
        return false;
    }

    if (st.st_size <
        static_cast<off_t>(
            sizeof(MetricMapFrameHeader) +
            MetricMapFrameHeader::kExpectedPayloadSize
        )) {
        close();
        return false;
    }

    mapped_size_ = static_cast<std::size_t>(st.st_size);

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

bool SynchronizedMetricMapTransport::consume(
    MetricMapFrame& output
) {
    if (mapped_memory_ == nullptr ||
        ready_semaphore_ == nullptr ||
        free_semaphore_ == nullptr) {
        return false;
    }

    while (::sem_wait(
        static_cast<sem_t*>(ready_semaphore_)
    ) != 0) {
        if (errno == EINTR) {
            continue;
        }
        return false;
    }

    const bool decoded = reader_.decode(
        static_cast<const std::uint8_t*>(mapped_memory_),
        mapped_size_,
        output
    );

    ::sem_post(
        static_cast<sem_t*>(free_semaphore_)
    );

    return decoded;
}

void SynchronizedMetricMapTransport::close() {
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
        ::munmap(mapped_memory_, mapped_size_);
        mapped_memory_ = nullptr;
        mapped_size_ = 0;
    }

    if (shm_fd_ != -1) {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
}

}  // namespace ugv