import ctypes
import os
import time
from multiprocessing import shared_memory

import numpy as np


class MetricMapPublisher:
    """
    Publishes a 160x160 metric cost map and observed mask
    through synchronized shared memory.
    """

    WIDTH = 160
    HEIGHT = 160
    PIXEL_COUNT = WIDTH * HEIGHT

    HEADER_SIZE = 40

    PAYLOAD_SIZE = (
        PIXEL_COUNT * np.dtype(np.uint8).itemsize
        + PIXEL_COUNT * np.dtype(np.uint8).itemsize
    )

    FRAME_SIZE = HEADER_SIZE + PAYLOAD_SIZE

    MAGIC = 0x5547564D
    VERSION = 1

    def __init__(
        self,
        name="ugv_metric_map",
        ready_semaphore="ugv_metric_map_ready",
        free_semaphore="ugv_metric_map_free",
    ):
        self.name = name
        self.ready_semaphore = ready_semaphore
        self.free_semaphore = free_semaphore

        self.shm = None
        self.ready = None
        self.free = None

        self.libc = ctypes.CDLL(None)

        self.libc.sem_open.restype = ctypes.c_void_p
        self.libc.sem_open.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int,
        ]

        self.libc.sem_close.argtypes = [
            ctypes.c_void_p,
        ]

        self.libc.sem_unlink.argtypes = [
            ctypes.c_char_p,
        ]

        self.libc.sem_wait.argtypes = [
            ctypes.c_void_p,
        ]

        self.libc.sem_post.argtypes = [
            ctypes.c_void_p,
        ]

        self.sequence = 0
        self.created = False

    @staticmethod
    def _semaphore_name(name):
        return (
            name
            if name.startswith("/")
            else "/" + name
        )

    def _open_semaphore(self, name, initial_value):
        semaphore_name = self._semaphore_name(
            name
        ).encode()

        ctypes.set_errno(0)

        handle = self.libc.sem_open(
            semaphore_name,
            os.O_CREAT | os.O_EXCL,
            0o600,
            initial_value,
        )

        if handle == ctypes.c_void_p(-1).value:
            error = ctypes.get_errno()

            if error != 17:
                raise OSError(
                    error,
                    os.strerror(error),
                )

            self.libc.sem_unlink(
                semaphore_name
            )

            ctypes.set_errno(0)

            handle = self.libc.sem_open(
                semaphore_name,
                os.O_CREAT | os.O_EXCL,
                0o600,
                initial_value,
            )

            if handle == ctypes.c_void_p(-1).value:
                error = ctypes.get_errno()

                raise OSError(
                    error,
                    os.strerror(error),
                )

        return handle

    def create(self):
        if self.created:
            raise RuntimeError(
                "Metric map publisher is already created"
            )

        self.shm = shared_memory.SharedMemory(
            name=self.name,
            create=True,
            size=self.FRAME_SIZE,
        )

        try:
            self.ready = self._open_semaphore(
                self.ready_semaphore,
                0,
            )

            self.free = self._open_semaphore(
                self.free_semaphore,
                1,
            )

            self.created = True

        except Exception:
            self.close()
            raise

    def publish(
        self,
        cost_map,
        observed,
        timestamp_ns=None,
    ):
        if not self.created:
            raise RuntimeError(
                "Metric map publisher is not created"
            )

        cost_map = np.asarray(
            cost_map,
            dtype=np.uint8,
        )

        observed = np.asarray(
            observed,
            dtype=np.uint8,
        )

        expected_shape = (
            self.HEIGHT,
            self.WIDTH,
        )

        if cost_map.shape != expected_shape:
            raise ValueError(
                f"Cost map shape must be "
                f"{expected_shape}, got "
                f"{cost_map.shape}"
            )

        if observed.shape != expected_shape:
            raise ValueError(
                f"Observed mask shape must be "
                f"{expected_shape}, got "
                f"{observed.shape}"
            )

        if timestamp_ns is None:
            timestamp_ns = time.monotonic_ns()

        if timestamp_ns <= 0:
            raise ValueError(
                "Timestamp must be positive"
            )

        while True:
            result = self.libc.sem_wait(
                self.free
            )

            if result == 0:
                break

            error = ctypes.get_errno()

            if error == 4:
                continue

            raise OSError(
                error,
                os.strerror(error),
            )

        try:
            self.sequence += 1

            header = np.ndarray(
                (self.HEADER_SIZE,),
                dtype=np.uint8,
                buffer=self.shm.buf,
                offset=0,
            )

            header[:] = 0

            header_view = memoryview(
                self.shm.buf
            )

            np.frombuffer(
                header_view,
                dtype="<u4",
                count=1,
                offset=0,
            )[0] = self.MAGIC

            np.frombuffer(
                header_view,
                dtype="<u2",
                count=1,
                offset=4,
            )[0] = self.VERSION

            np.frombuffer(
                header_view,
                dtype="<u2",
                count=1,
                offset=6,
            )[0] = self.HEADER_SIZE

            np.frombuffer(
                header_view,
                dtype="<u4",
                count=1,
                offset=8,
            )[0] = self.WIDTH

            np.frombuffer(
                header_view,
                dtype="<u4",
                count=1,
                offset=12,
            )[0] = self.HEIGHT

            np.frombuffer(
                header_view,
                dtype="<u8",
                count=1,
                offset=16,
            )[0] = self.sequence

            np.frombuffer(
                header_view,
                dtype="<u8",
                count=1,
                offset=24,
            )[0] = timestamp_ns

            np.frombuffer(
                header_view,
                dtype="<u4",
                count=1,
                offset=32,
            )[0] = self.PAYLOAD_SIZE

            cost_destination = np.ndarray(
                (self.PIXEL_COUNT,),
                dtype=np.uint8,
                buffer=self.shm.buf,
                offset=self.HEADER_SIZE,
            )

            observed_destination = np.ndarray(
                (self.PIXEL_COUNT,),
                dtype=np.uint8,
                buffer=self.shm.buf,
                offset=self.HEADER_SIZE
                + self.PIXEL_COUNT,
            )

            cost_destination[:] = (
                cost_map.reshape(-1)
            )

            observed_destination[:] = (
                observed.reshape(-1)
            )

            self.libc.sem_post(
                self.ready
            )

        except Exception:
            self.libc.sem_post(
                self.free
            )
            raise

        return self.sequence

    def close(self):
        if self.ready is not None:
            self.libc.sem_close(
                self.ready
            )
            self.ready = None

        if self.free is not None:
            self.libc.sem_close(
                self.free
            )
            self.free = None

        if self.shm is not None:
            self.shm.close()
            self.shm = None

        self.created = False

    def unlink(self):
        self.libc.sem_unlink(
            self._semaphore_name(
                self.ready_semaphore
            ).encode()
        )

        self.libc.sem_unlink(
            self._semaphore_name(
                self.free_semaphore
            ).encode()
        )

        try:
            stale = shared_memory.SharedMemory(
                name=self.name
            )
            stale.close()
            stale.unlink()
        except FileNotFoundError:
            pass