import ctypes
import ctypes.util
import os
import struct
import time
from multiprocessing import shared_memory

import numpy as np


class SharedMemoryPublisher:
    MAGIC = 0x55475650
    VERSION = 1

    WIDTH = 512
    HEIGHT = 512

    HEADER_FORMAT = "<IHHIIQQI"
    HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

    SEGMENTATION_SIZE = WIDTH * HEIGHT
    FLOAT_ARRAY_SIZE = WIDTH * HEIGHT * 4

    PAYLOAD_SIZE = (
        SEGMENTATION_SIZE
        + FLOAT_ARRAY_SIZE
        + FLOAT_ARRAY_SIZE
        + FLOAT_ARRAY_SIZE
    )

    FRAME_SIZE = HEADER_SIZE + PAYLOAD_SIZE

    SEM_FAILED = ctypes.c_void_p(-1).value

    def __init__(
        self,
        name="ugv_perception_frame",
        ready_semaphore="ugv_perception_ready",
        free_semaphore="ugv_perception_free",
    ):
        self.name = name

        self.ready_semaphore_name = (
            "/" + ready_semaphore.lstrip("/")
        )

        self.free_semaphore_name = (
            "/" + free_semaphore.lstrip("/")
        )

        self.shared_memory = None
        self.ready_semaphore = None
        self.free_semaphore = None
        self.sequence = 0

        libc_name = ctypes.util.find_library("c")

        if libc_name is None:
            raise RuntimeError("Unable to locate libc")

        self.libc = ctypes.CDLL(
            libc_name,
            use_errno=True,
        )

        self.libc.sem_open.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int
            
        ]
        self.libc.sem_open.restype = ctypes.c_void_p

        self.libc.sem_wait.argtypes = [ctypes.c_void_p]
        self.libc.sem_wait.restype = ctypes.c_int

        self.libc.sem_post.argtypes = [ctypes.c_void_p]
        self.libc.sem_post.restype = ctypes.c_int

        self.libc.sem_close.argtypes = [ctypes.c_void_p]
        self.libc.sem_close.restype = ctypes.c_int

        self.libc.sem_unlink.argtypes = [ctypes.c_char_p]
        self.libc.sem_unlink.restype = ctypes.c_int

    def _create_semaphores(self):
        ready_name = self.ready_semaphore_name.encode()
        free_name = self.free_semaphore_name.encode()

        self.libc.sem_unlink(ready_name)
        self.libc.sem_unlink(free_name)

        self.ready_semaphore = self.libc.sem_open(
            ready_name,
            os.O_CREAT | os.O_EXCL,
            0o600,
            0,
        )

        if self.ready_semaphore == self.SEM_FAILED:
            self.ready_semaphore = None
            raise RuntimeError(
                "Unable to create ready semaphore"
            )

        self.free_semaphore = self.libc.sem_open(
            free_name,
            os.O_CREAT | os.O_EXCL,
            0o600,
            1,
        )

        if self.free_semaphore == self.SEM_FAILED:
            self.free_semaphore = None

            self.libc.sem_close(
                self.ready_semaphore
            )

            self.ready_semaphore = None
            self.libc.sem_unlink(ready_name)

            raise RuntimeError(
                "Unable to create free semaphore"
            )

    def _wait_free(self):
        while True:
            result = self.libc.sem_wait(
                self.free_semaphore
            )

            if result == 0:
                return

            error = ctypes.get_errno()

            if error != 4:
                raise RuntimeError(
                    f"sem_wait failed: errno={error}"
                )

    def _signal_ready(self):
        if self.libc.sem_post(
            self.ready_semaphore
        ) != 0:
            error = ctypes.get_errno()

            raise RuntimeError(
                f"sem_post failed: errno={error}"
            )

    def create(self):
        if self.shared_memory is not None:
            raise RuntimeError(
                "Shared memory is already open"
            )

        self.shared_memory = shared_memory.SharedMemory(
            name=self.name,
            create=True,
            size=self.FRAME_SIZE,
        )

        self.shared_memory.buf[
            :self.FRAME_SIZE
        ] = bytearray(self.FRAME_SIZE)

        try:
            self._create_semaphores()
        except Exception:
            self.shared_memory.close()
            self.shared_memory.unlink()
            self.shared_memory = None
            raise

        self.sequence = 0

    def publish(
        self,
        segmentation,
        confidence,
        depth,
        roughness,
    ):
        if self.shared_memory is None:
            raise RuntimeError(
                "Shared memory is not open"
            )

        segmentation = np.asarray(
            segmentation,
            dtype=np.uint8,
        )

        confidence = np.asarray(
            confidence,
            dtype=np.float32,
        )

        depth = np.asarray(
            depth,
            dtype=np.float32,
        )

        roughness = np.asarray(
            roughness,
            dtype=np.float32,
        )

        expected_shape = (
            self.HEIGHT,
            self.WIDTH,
        )

        if segmentation.shape != expected_shape:
            raise ValueError(
                f"Invalid segmentation shape: "
                f"{segmentation.shape}"
            )

        if confidence.shape != expected_shape:
            raise ValueError(
                f"Invalid confidence shape: "
                f"{confidence.shape}"
            )

        if depth.shape != expected_shape:
            raise ValueError(
                f"Invalid depth shape: "
                f"{depth.shape}"
            )

        if roughness.shape != expected_shape:
            raise ValueError(
                f"Invalid roughness shape: "
                f"{roughness.shape}"
            )

        if not np.isfinite(confidence).all():
            raise ValueError(
                "Confidence contains non-finite values"
            )

        if not np.isfinite(depth).all():
            raise ValueError(
                "Depth contains non-finite values"
            )

        if not np.isfinite(roughness).all():
            raise ValueError(
                "Roughness contains non-finite values"
            )

        segmentation = np.ascontiguousarray(
            segmentation,
            dtype=np.uint8,
        )

        confidence = np.ascontiguousarray(
            confidence,
            dtype=np.float32,
        )

        depth = np.ascontiguousarray(
            depth,
            dtype=np.float32,
        )

        roughness = np.ascontiguousarray(
            roughness,
            dtype=np.float32,
        )

        self._wait_free()

        self.sequence += 1

        timestamp_ns = time.monotonic_ns()

        header = struct.pack(
            self.HEADER_FORMAT,
            self.MAGIC,
            self.VERSION,
            self.HEADER_SIZE,
            self.WIDTH,
            self.HEIGHT,
            self.sequence,
            timestamp_ns,
            self.PAYLOAD_SIZE,
        )

        buffer = self.shared_memory.buf

        offset = 0

        buffer[
            offset:
            offset + self.HEADER_SIZE
        ] = header

        offset += self.HEADER_SIZE

        buffer[
            offset:
            offset + self.SEGMENTATION_SIZE
        ] = segmentation.tobytes(order="C")

        offset += self.SEGMENTATION_SIZE

        buffer[
            offset:
            offset + self.FLOAT_ARRAY_SIZE
        ] = confidence.tobytes(order="C")

        offset += self.FLOAT_ARRAY_SIZE

        buffer[
            offset:
            offset + self.FLOAT_ARRAY_SIZE
        ] = depth.tobytes(order="C")

        offset += self.FLOAT_ARRAY_SIZE

        buffer[
            offset:
            offset + self.FLOAT_ARRAY_SIZE
        ] = roughness.tobytes(order="C")

        self._signal_ready()

        return self.sequence

    def close(self):
        if self.ready_semaphore is not None:
            self.libc.sem_close(
                self.ready_semaphore
            )
            self.ready_semaphore = None

        if self.free_semaphore is not None:
            self.libc.sem_close(
                self.free_semaphore
            )
            self.free_semaphore = None

        if self.shared_memory is not None:
            self.shared_memory.close()
            self.shared_memory = None

    def unlink(self):
        if self.shared_memory is not None:
            self.shared_memory.unlink()

        self.libc.sem_unlink(
            self.ready_semaphore_name.encode()
        )

        self.libc.sem_unlink(
            self.free_semaphore_name.encode()
        )

    def __enter__(self):
        self.create()
        return self

    def __exit__(
        self,
        exc_type,
        exc_value,
        traceback,
    ):
        self.close()
        self.unlink()