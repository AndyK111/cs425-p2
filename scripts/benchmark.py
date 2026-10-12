#!/usr/bin/env python3
"""Generate the P2 results table: python3 scripts/benchmark.py.

Edit the settings below, then run from the host or inside the dev container.
The assignment defaults run twelve transfers sequentially and take about
15 minutes. Each run must succeed and match the input before it is recorded.
The script prints a Markdown table and saves it alongside raw CSV timings,
input/output files, and logs in a unique directory under OUTPUT_DIR.
"""

import csv
import errno
import math
import os
from pathlib import Path
import random
import signal
import socket
import statistics
import subprocess
import sys
import tempfile
import time

# -----------------------------------------------------------------------------
# All adjustable settings and dials. Assignment defaults are selected below.
# -----------------------------------------------------------------------------
ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "build/release/myapp"
RELAY = ROOT / "cs425_relay.py"
BUILD_COMMAND = ["make", "-B", "release"]  # [] skips building an existing binary.
PYTHON = sys.executable
OUTPUT_DIR = ROOT / "build/benchmark"

HOST = "127.0.0.1"  # Relay, sender, and receiver all run locally.
PORT = None  # None selects an available port in AUTO_PORT_RANGE.
AUTO_PORT_RANGE = (20000, 60000)
PORT_SELECTION_ATTEMPTS = 100
SESSION_PREFIX = "bench"  # Lowercase letters, digits, and hyphens only.

FILE_SIZE = 1024 * 1024  # Bytes; assignment requires exactly 1 MiB.
REPEATS = 3
DELAY_MS = 50  # One-way delay; assignment requires 50 ms (100 ms round trip).
TIMEOUT_MS = 250  # Sender's retransmission timeout.
SEED = 7  # None uses unseeded relay damage; the RNG continues across all runs.
# Each case is (window size, loss probability, corruption probability, duplication probability).
CASES = [
    (1, 0, 0, 0),
    (16, 0, 0, 0),
    (1, 0.05, 0, 0),
    (16, 0.05, 0, 0),
]

STARTUP_TIMEOUT_SECONDS = 10
TRANSFER_TIMEOUT_SECONDS = 600  # Overall sender limit, separate from TIMEOUT_MS.
RECEIVER_EXIT_TIMEOUT_SECONDS = 10  # Includes the receiver's two-second linger.
SHUTDOWN_TIMEOUT_SECONDS = 5
POLL_SECONDS = 0.05

RUN_DIRECTORY_PREFIX = "run-"
INPUT_FILENAME = "input.bin"
RECEIVED_FILENAME = "received.bin"
RELAY_LOG_FILENAME = "relay.log"
SENDER_LOG_FILENAME = "sender.log"
RECEIVER_LOG_FILENAME = "receiver.log"
CSV_FILENAME = "runs.csv"
REPORT_FILENAME = "results.md"
CSV_DECIMAL_PLACES = 6
REPORT_DECIMAL_PLACES = 3
# -----------------------------------------------------------------------------

# These are fixed protocol limits, not benchmark settings.
MAX_INPUT_SIZE = 16 * 1024 * 1024
MAX_WINDOW = 64
MAX_TIMEOUT_MS = (1 << 32) - 1


def validate_settings():
    """Reject invalid dials before building, starting processes, or writing results."""
    if not isinstance(REPEATS, int) or REPEATS < 1 or not CASES:
        raise ValueError("Use at least one case and one repetition")
    if not isinstance(FILE_SIZE, int) or not 0 <= FILE_SIZE <= MAX_INPUT_SIZE:
        raise ValueError("FILE_SIZE must be an integer from 0 to 16 MiB")
    if not isinstance(TIMEOUT_MS, int) or not 1 <= TIMEOUT_MS <= MAX_TIMEOUT_MS:
        raise ValueError("TIMEOUT_MS must be a positive 32-bit integer")
    if not math.isfinite(DELAY_MS) or DELAY_MS < 0:
        raise ValueError("DELAY_MS must be finite and nonnegative")
    if PORT is not None and (not isinstance(PORT, int) or not 1 <= PORT <= 65535):
        raise ValueError("PORT must be None or an integer from 1 to 65535")
    if len(AUTO_PORT_RANGE) != 2 or not 1 <= AUTO_PORT_RANGE[0] <= AUTO_PORT_RANGE[1] <= 65535:
        raise ValueError("AUTO_PORT_RANGE must be an inclusive range within 1..65535")
    if not isinstance(PORT_SELECTION_ATTEMPTS, int) or PORT_SELECTION_ATTEMPTS < 1:
        raise ValueError("PORT_SELECTION_ATTEMPTS must be a positive integer")
    if SEED is not None and not isinstance(SEED, int):
        raise ValueError("SEED must be None or an integer")
    for name, value in (
        ("STARTUP_TIMEOUT_SECONDS", STARTUP_TIMEOUT_SECONDS),
        ("TRANSFER_TIMEOUT_SECONDS", TRANSFER_TIMEOUT_SECONDS),
        ("RECEIVER_EXIT_TIMEOUT_SECONDS", RECEIVER_EXIT_TIMEOUT_SECONDS),
        ("SHUTDOWN_TIMEOUT_SECONDS", SHUTDOWN_TIMEOUT_SECONDS),
        ("POLL_SECONDS", POLL_SECONDS),
    ):
        if not math.isfinite(value) or value <= 0:
            raise ValueError(f"{name} must be finite and positive")
    for precision in (CSV_DECIMAL_PLACES, REPORT_DECIMAL_PLACES):
        if not isinstance(precision, int) or precision < 0:
            raise ValueError("Decimal precision settings must be nonnegative integers")
    longest_session = f"{SESSION_PREFIX}-{len(CASES)}-{REPEATS}"
    if not SESSION_PREFIX or len(longest_session) > 32 or any(
        character not in "abcdefghijklmnopqrstuvwxyz0123456789-"
        for character in SESSION_PREFIX
    ):
        raise ValueError("SESSION_PREFIX must produce valid session names of at most 32 characters")
    for case in CASES:
        if len(case) != 4:
            raise ValueError("Each CASES entry must contain window, loss, corruption, duplication")
        window, *rates = case
        if not isinstance(window, int) or not 1 <= window <= MAX_WINDOW:
            raise ValueError(f"Invalid window size in case {case}")
        if any(not math.isfinite(rate) or not 0 <= rate <= 0.5 for rate in rates):
            raise ValueError(f"Damage probabilities must be between 0 and 0.5: {case}")
    filenames = (
        INPUT_FILENAME, RECEIVED_FILENAME, RELAY_LOG_FILENAME, SENDER_LOG_FILENAME,
        RECEIVER_LOG_FILENAME, CSV_FILENAME, REPORT_FILENAME,
    )
    if len(set(filenames)) != len(filenames) or any(
        not name or name in (".", "..") or Path(name).name != name for name in filenames
    ):
        raise ValueError("Output filenames must be distinct names without directories")
    if not RELAY.is_file():
        raise ValueError(f"Relay script does not exist: {RELAY}")


def choose_port():
    """Select a local UDP port; the relay reports any intervening bind collision."""
    if PORT is not None:
        return PORT
    rng = random.SystemRandom()
    for _ in range(PORT_SELECTION_ATTEMPTS):
        port = rng.randint(*AUTO_PORT_RANGE)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            try:
                sock.bind((HOST, port))
                return port
            except OSError as error:
                if error.errno != errno.EADDRINUSE:
                    raise
    raise RuntimeError("Could not find a free local UDP port")


def stop(process, graceful_relay=False):
    """Stop and reap a child; SIGINT lets the relay print its session statistics."""
    if process is None:
        return
    if process.poll() is None:
        try:
            if graceful_relay:
                process.send_signal(signal.SIGINT)
            else:
                process.terminate()
        except ProcessLookupError:
            pass
        try:
            process.wait(timeout=SHUTDOWN_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    else:
        process.wait()


def wait_ready(process, ready, log):
    """Wait for a concrete readiness indication instead of a fixed startup sleep."""
    deadline = time.perf_counter() + STARTUP_TIMEOUT_SECONDS
    while time.perf_counter() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"Process exited with {process.returncode}; see {log}")
        if ready():
            return
        time.sleep(POLL_SECONDS)
    raise RuntimeError(f"Startup timed out; see {log}")


def transfer(folder, port, case, case_index, repetition, data):
    """Time one sender, wait for both endpoints, and compare every delivered byte."""
    window, loss, corrupt, dup = case
    session = f"{SESSION_PREFIX}-{case_index}-{repetition}"
    output = folder / RECEIVED_FILENAME
    receiver_log = folder / RECEIVER_LOG_FILENAME
    sender_log = folder / SENDER_LOG_FILENAME
    common = ["-s", session, "-p", str(port)]
    sender = receiver = None
    with receiver_log.open("w") as recv_log, sender_log.open("w") as send_log:
        try:
            receiver = subprocess.Popen(
                [str(APP), "recv", *common, HOST, str(output)],
                stdout=recv_log, stderr=subprocess.STDOUT, cwd=ROOT,
            )
            # This application creates its output only after registration succeeds.
            wait_ready(receiver, output.exists, receiver_log)
            # Fixed decimal notation matches the relay's probability grammar.
            sender_command = [
                str(APP), "send", *common, "-w", str(window), "-T", str(TIMEOUT_MS),
                "-l", format(loss, ".17f"), "-c", format(corrupt, ".17f"),
                "-d", format(dup, ".17f"), HOST, str(folder.parent / INPUT_FILENAME),
            ]
            start = time.perf_counter()
            sender = subprocess.Popen(
                sender_command, stdout=send_log, stderr=subprocess.STDOUT, cwd=ROOT,
            )
            try:
                sender_status = sender.wait(timeout=TRANSFER_TIMEOUT_SECONDS)
            except subprocess.TimeoutExpired as error:
                raise RuntimeError(f"Sender timed out; see {sender_log}") from error
            elapsed = time.perf_counter() - start  # Excludes receiver startup/linger.
            if sender_status != 0:
                raise RuntimeError(f"Sender exited with {sender_status}; see {sender_log}")
            try:
                receiver_status = receiver.wait(timeout=RECEIVER_EXIT_TIMEOUT_SECONDS)
            except subprocess.TimeoutExpired as error:
                raise RuntimeError(f"Receiver did not exit; see {receiver_log}") from error
            if receiver_status != 0:
                raise RuntimeError(f"Receiver exited with {receiver_status}; see {receiver_log}")
            if output.read_bytes() != data:
                raise RuntimeError(f"Received bytes differ from input: {output}")
            return elapsed
        finally:
            stop(sender)
            stop(receiver)


def format_results_table(results):
    """Format means and KiB/s using verified timings only, without premature rounding."""
    rows = [
        "| Window | Loss | Corrupt | Dup | Mean time (s) | Throughput (KiB/s) |",
        "|---|---|---|---|---|---|",
    ]
    for case, times in results:
        mean = statistics.mean(times)
        throughput = FILE_SIZE / 1024 / mean
        rows.append(
            "| " + " | ".join(map(str, case))
            + f" | {mean:.{REPORT_DECIMAL_PLACES}f}"
            + f" | {throughput:.{REPORT_DECIMAL_PLACES}f} |"
        )
    return "\n".join(rows)


def write_results_report(folder, results):
    """Checkpoint complete cases so an interrupted benchmark retains its table."""
    report = (
        f"Input: {FILE_SIZE} bytes; {REPEATS} repeats per case; relay delay: "
        f"{DELAY_MS} ms; sender timeout: {TIMEOUT_MS} ms; seed: {SEED}.\n\n"
        f"Completed cases: {len(results)}/{len(CASES)}. "
        "Times measure sender wall time, excluding receiver startup and linger.\n"
        "Throughput = input bytes / 1024 / mean sender time.\n\n"
        + format_results_table(results) + "\n"
    )
    (folder / REPORT_FILENAME).write_text(report, encoding="utf-8")
    return report


def main():
    """Build, run cases sequentially through one owned relay, and preserve all evidence."""
    validate_settings()
    if BUILD_COMMAND:
        subprocess.run(BUILD_COMMAND, cwd=ROOT, check=True)
    if not APP.is_file() or not os.access(APP, os.X_OK):
        raise ValueError(f"Application is missing or not executable: {APP}")
    port = choose_port()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    folder = Path(tempfile.mkdtemp(prefix=RUN_DIRECTORY_PREFIX, dir=OUTPUT_DIR))
    data = os.urandom(FILE_SIZE)
    (folder / INPUT_FILENAME).write_bytes(data)
    print(f"Results: {folder}\nRelay: {HOST}:{port}, delay {DELAY_MS} ms", flush=True)
    relay_log = folder / RELAY_LOG_FILENAME
    command = [
        PYTHON, str(RELAY), "--host", HOST, "--port", str(port), "--delay", str(DELAY_MS),
    ]
    if SEED is not None:
        command += ["--seed", str(SEED)]
    relay = None
    results = []
    write_results_report(folder, results)
    with relay_log.open("w") as log, (folder / CSV_FILENAME).open("w", newline="") as runs:
        writer = csv.writer(runs)
        writer.writerow(["window", "loss", "corrupt", "dup", "repeat", "seconds", "identical"])
        runs.flush()
        try:
            relay = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
            wait_ready(relay, lambda: "listening on udp" in relay_log.read_text(), relay_log)
            for index, case in enumerate(CASES, 1):
                times = []
                for repetition in range(1, REPEATS + 1):
                    run = folder / f"case-{index}-repeat-{repetition}"
                    run.mkdir()
                    print(f"Case {case}, repeat {repetition}/{REPEATS}...", flush=True)
                    if relay.poll() is not None:
                        raise RuntimeError(f"Relay stopped; see {relay_log}")
                    elapsed = transfer(run, port, case, index, repetition, data)
                    times.append(elapsed)
                    writer.writerow([*case, repetition, f"{elapsed:.{CSV_DECIMAL_PLACES}f}", True])
                    runs.flush()
                    print(f"  {elapsed:.{REPORT_DECIMAL_PLACES}f} s; files identical", flush=True)
                results.append((case, times))
                write_results_report(folder, results)
        finally:
            stop(relay, graceful_relay=True)
    print("\n" + write_results_report(folder, results), flush=True)
    return folder


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped; logs, completed timings, and partial results are preserved.", file=sys.stderr)
        sys.exit(130)
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Benchmark failed: {error}", file=sys.stderr)
        sys.exit(1)
