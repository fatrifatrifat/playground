"""
Mandatory file for tests that require a setup using pytest
"""

import os
import subprocess

import grpc
import pytest

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_ENGINE_BIN = os.environ.get(
    "ENGINE_BIN",
    os.path.join(_REPO_ROOT, "build", "engine-cpp", "src", "trading_engine"),
)
_SERVER = "localhost:50051"


def _wait_for_engine(address: str, timeout: float = 10.0) -> None:
    ch = grpc.insecure_channel(address)
    try:
        grpc.channel_ready_future(ch).result(timeout=timeout)
    finally:
        ch.close()


@pytest.fixture
def engine_proc():
    if not os.path.isfile(_ENGINE_BIN):
        pytest.skip(
            f"engine binary not found at {_ENGINE_BIN} — "
            "run: cmake --build build -j"
        )

    proc = subprocess.Popen(
        [_ENGINE_BIN],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    try:
        _wait_for_engine(_SERVER, timeout=10.0)
    except Exception:
        proc.kill()
        proc.wait()
        pytest.fail("engine did not become ready within 10 seconds")

    yield proc

    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
