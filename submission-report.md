# Submission Report

- Submission generated at 10/16/2026 at 06:13:18

- Machine info: Linux runnervm8k3b6 6.17.0-1022-azure #22-Ubuntu SMP Mon Jul 27 17:24:03 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux

## Note to Students

Please read this report carefully before submission.
Ensure that all sections are complete and accurate.
Look for any errors in the build or test outputs.
If you find any issues, correct them before submitting.
Post any questions on the class discussion board for help.


---

## README

# Project 2 - Reliable Data Transfer 

- Name: Andy Kempf
- Email: AndyKempf@u.boisestate.edu
- Class: 425-001

## Design

The project is seperated into 3-layers, as per specified in the PDF files. The layers are as follows:
- Packet Layer : Defines the type and size of packets, computed the checksum for each packet, encodes packets into wire bytes, and decodes/validates incoming datagrams. This layer generate the information that a packet is.
- Protocol Layer : Implements Go-Back-N behavior. While individual packets are sent, the Protocol Layer will track the status of those packets, in what order they are acknowledged, after how logn they should be resent, etc... This layer orchestrates the file transfer and ensures that the data is all received. If the sender's timer expires before an acknowledgement advances the window, the sender retransmits all outstanding packets.
- I/O Layer : This is the component that actually sends the packets. While the packet layer handles generating packet data, the protocol layer orchestrates which packets should be sent, and ensures they are all properly received, the I/O layer includes the machinery used to actually send the packets over the "network". It is responsible for allocating the actual resources (sockets, files, etc...) used to transmit the packet data. It also where the top-level interaction for the software comes from, it reads command-line arguments and opens the UDP socket needed, and registers that socket with the relay. 

The three layer split exists to consolidate responsibilities, for instance, the packet data layer doesn't need to know about how sockets are handled, and the Protocol Layer doesnt need to know what exactly is contained in each particular packet.

The three layer split also allows us to test each component individually, without needing to simulate the entire pipeline.

Files in each layer:
- Packet Layer: `packets.h`/`packets.c`
- Protocol Layer: `gbn_protocol.h`/`gbn_protocol.c`
- I/O Layer: `transfer_io.h`/`transfer_io.c`

## Results

| Window | Loss | Corrupt | Dup | Mean time (s) | Throughput (KiB/s) |
|---|---|---|---|---|---|
| 1 | 0 | 0 | 0 | 103.567 | 9.887 |
| 16 | 0 | 0 | 0 | 6.631 | 154.415 |
| 1 | 0.05 | 0 | 0 | 129.399 | 7.914 |
| 16 | 0.05 | 0 | 0 | 23.623 | 43.347 |

## Results Analysis

With a window of 1 and no loss, the mean transfer time was 103.567 seconds. Dividing this by the 1025 packets gives an estimated round-trip time of `101.04 ms`. The relay contributes 100ms, which leaves an extra 1.04 ms spend on operating-system effects, relay processing, and socket/file delays. Increasing the window to 16 reduced the mean time to 6.631 seconds, a `15.62` times speedup. This is proportional to the window size because the larger window allows the sender to keep multiple packets in flight while waiting for `ACK`s to come back. 

With 5% loss, the window-1 time increased from 103.567 to 129.399 seconds, approximated a 25% increase. The window-16 time increased from 6.631 to 23.623 seconds, an almost 256% increase. Lost packets or acknowledgements can cause the sender's timer to expire. Go-Back-N then tells the sender to retransmit every outstanding packet within the current window. Window 1 will retransmit only a single packets, while window 16 could retransmit multiple. The receiver also discard packets that arrive beyond a gap, requiring them to be sent again. This makes loss proportionally more expensive when the window size is larger, though a window size of 16 remains much faster. 

## Testing

The Unity tests cover packet encoding and decoding, checksum examples, corrupted packets, malformed headers, and invalid lengths. 

Receiver tests check ordering, duplicated, gaps, FIN, and timeouts.

Sender tests check window limits, cumulative acknowledgements, retransmission, and failure after ten timeouts with no progress.

Complete transers are tested usinga f ake clock and in-memory chanel with 20% loss (seeded for consistent behavior). Delivered bytes are compared against the original input, Tests include empty files and files with sizes that are exact multiples of 1024 bytes.

All 17 of the tests pass and line coverage is 100%.

## known Bugs or Issues

No known bugs were found in unit testing, sanitizer checking, or relay transfers.

## Experience

I began the project by using the printscreen features to turn the assignments canvas page into a PDF. I include the PDF in the project directory and have Codex review it in plan mode. I go over with codex how I want files seggregated, how I want the code formatted and documented, and introduce it to personal style choices like the use of regions and seperating brackets and long paran arguments onto their own newlines.

For the table results, I asked codex to create a python script that would automatically run all of the benchmark scenarios then export the table to a markdown file.
---


## Build Output

This section was generated by running `make all` in the project root directory.

```bash
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/transfer_io.c -o build/debug/transfer_io.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug/lab.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/gbn_protocol.c -o build/debug/gbn_protocol.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/packets.c -o build/debug/packets.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug/main.c.o
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address build/debug/transfer_io.c.o build/debug/lab.c.o build/debug/gbn_protocol.c.o build/debug/packets.c.o build/debug/main.c.o -o build/debug/myapp_d -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/transfer_io.c -o build/release/transfer_io.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/lab.c -o build/release/lab.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/gbn_protocol.c -o build/release/gbn_protocol.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/packets.c -o build/release/packets.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/main.c -o build/release/main.c.o
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion build/release/transfer_io.c.o build/release/lab.c.o build/release/gbn_protocol.c.o build/release/packets.c.o build/release/main.c.o -o build/release/myapp 
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/transfer_io.c -o build/tests/transfer_io.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/lab.c -o build/tests/lab.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/gbn_protocol.c -o build/tests/gbn_protocol.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/packets.c -o build/tests/packets.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/main.c -o build/tests/main.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/transfer-io-test.c -o build/tests/transfer-io-test.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/lab-test.c -o build/tests/lab-test.c.o
mkdir -p build/tests/harness/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/harness/unity.c -o build/tests/harness/unity.c.o
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage build/tests/transfer_io.c.o build/tests/lab.c.o build/tests/gbn_protocol.c.o build/tests/packets.c.o build/tests/main.c.o build/tests/transfer-io-test.c.o build/tests/lab-test.c.o build/tests/harness/unity.c.o -o build/tests/myapp_t -fprofile-arcs -ftest-coverage
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
make[1]: Entering directory '/home/runner/work/cs425-p2/cs425-p2'
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/transfer_io.c -o build/debug-test/transfer_io.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug-test/lab.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/gbn_protocol.c -o build/debug-test/gbn_protocol.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/packets.c -o build/debug-test/packets.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug-test/main.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/transfer-io-test.c -o build/debug-test/transfer-io-test.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/lab-test.c -o build/debug-test/lab-test.c.o
mkdir -p build/debug-test/harness/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/harness/unity.c -o build/debug-test/harness/unity.c.o
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address build/debug-test/transfer_io.c.o build/debug-test/lab.c.o build/debug-test/gbn_protocol.c.o build/debug-test/packets.c.o build/debug-test/main.c.o build/debug-test/transfer-io-test.c.o build/debug-test/lab-test.c.o build/debug-test/harness/unity.c.o -o build/debug-test/myapp_td -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p2/cs425-p2'
Builds completed. You can run the application with: ./build/release/myapp
You can run the debug build with: ./build/debug/myapp_d
You can run the test build with: ./build/tests/myapp_t
You can run the debug-test build with: ./build/debug-test/myapp_td
```

---

## Coverage Report

This section was generated by running `make report` in the project root directory.

```bash
Invalid sender configuration
Invalid receiver configuration
Invalid receiver configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Relay refused registration: test refusal
Relay registration timed out after five attempts
Transfer failed after ten timeouts without progress
Relay socket failed
getaddrinfo: Name or service not known
input file: No such file or directory
read input: Is a directory
Input exceeds 16 MiB
Relay refused registration: test refusal
getaddrinfo: Name or service not known
output file: No such file or directory
close output: No space left on device
Receiver idle timeout or input limit exceeded
tests/lab-test.c:665:test_checksum_examples:PASS
tests/lab-test.c:666:test_packet_worked_examples_and_round_trip:PASS
tests/lab-test.c:667:test_packet_invalid_arguments_and_fields:PASS
tests/lab-test.c:668:test_receiver_order_duplicates_gaps_and_finish:PASS
tests/lab-test.c:669:test_receiver_idle_and_packet_deadline:PASS
tests/lab-test.c:670:test_receiver_input_limit_and_saturated_clock:PASS
tests/lab-test.c:671:test_protocol_null_arguments:PASS
tests/lab-test.c:672:test_sender_window_acknowledgments_and_fin:PASS
tests/lab-test.c:673:test_sender_empty_partial_and_window_limits:PASS
tests/lab-test.c:674:test_sender_failure_and_progress_reset:PASS
tests/lab-test.c:675:test_lost_finish_acknowledgment_recovery:PASS
tests/lab-test.c:676:test_seeded_damaged_transfers:PASS
Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]
                  [-c corrupt] [-d dup] [-p port] <relay> <file>
       myapp recv -s <session> [-p port] <relay> <file>
  -s <session>     1 to 32 characters from a-z, 0-9 and -
  -w <window>      window in packets, 1 to 64 (default: 8)
  -T <timeout-ms>  positive retransmission timeout (default: 250)
  -l <loss>        drop probability, 0 to 0.5 (default: 0)
  -c <corrupt>     corruption probability, 0 to 0.5 (default: 0)
  -d <dup>         duplication probability, 0 to 0.5 (default: 0)
  -p <port>        relay port, 1 to 65535 (default: 4250)
tests/lab-test.c:514:test_cli_defaults_and_options:PASS
tests/lab-test.c:515:test_cli_invalid_arguments:PASS
tests/lab-test.c:516:test_io_invalid_configuration:PASS
tests/lab-test.c:517:test_io_sender_success_retries_and_failures:PASS
tests/lab-test.c:518:test_io_receiver_success_and_failures:PASS

-----------------------
17 Tests 0 Failures 0 Ignored 
OK
./build/tests/myapp_t
Invalid sender configuration
Invalid receiver configuration
Invalid receiver configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Relay refused registration: test refusal
Relay registration timed out after five attempts
Transfer failed after ten timeouts without progress
Relay socket failed
getaddrinfo: Name or service not known
input file: No such file or directory
read input: Is a directory
Input exceeds 16 MiB
Relay refused registration: test refusal
getaddrinfo: Name or service not known
output file: No such file or directory
close output: No space left on device
Receiver idle timeout or input limit exceeded
tests/lab-test.c:665:test_checksum_examples:PASS
tests/lab-test.c:666:test_packet_worked_examples_and_round_trip:PASS
tests/lab-test.c:667:test_packet_invalid_arguments_and_fields:PASS
tests/lab-test.c:668:test_receiver_order_duplicates_gaps_and_finish:PASS
tests/lab-test.c:669:test_receiver_idle_and_packet_deadline:PASS
tests/lab-test.c:670:test_receiver_input_limit_and_saturated_clock:PASS
tests/lab-test.c:671:test_protocol_null_arguments:PASS
tests/lab-test.c:672:test_sender_window_acknowledgments_and_fin:PASS
tests/lab-test.c:673:test_sender_empty_partial_and_window_limits:PASS
tests/lab-test.c:674:test_sender_failure_and_progress_reset:PASS
tests/lab-test.c:675:test_lost_finish_acknowledgment_recovery:PASS
tests/lab-test.c:676:test_seeded_damaged_transfers:PASS
Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]
                  [-c corrupt] [-d dup] [-p port] <relay> <file>
       myapp recv -s <session> [-p port] <relay> <file>
  -s <session>     1 to 32 characters from a-z, 0-9 and -
  -w <window>      window in packets, 1 to 64 (default: 8)
  -T <timeout-ms>  positive retransmission timeout (default: 250)
  -l <loss>        drop probability, 0 to 0.5 (default: 0)
  -c <corrupt>     corruption probability, 0 to 0.5 (default: 0)
  -d <dup>         duplication probability, 0 to 0.5 (default: 0)
  -p <port>        relay port, 1 to 65535 (default: 4250)
tests/lab-test.c:514:test_cli_defaults_and_options:PASS
tests/lab-test.c:515:test_cli_invalid_arguments:PASS
tests/lab-test.c:516:test_io_invalid_configuration:PASS
tests/lab-test.c:517:test_io_sender_success_retries_and_failures:PASS
tests/lab-test.c:518:test_io_receiver_success_and_failures:PASS

-----------------------
17 Tests 0 Failures 0 Ignored 
OK
mkdir -p ./build/report/html
mkdir -p ./build/report/txt
gcovr -r . --html --html-details --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$' -o ./build/report/html/coverage_report.html
(INFO) Reading coverage data...

(INFO) Writing coverage report...

gcovr -r . --txt                 --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$'
(INFO) Reading coverage data...

(INFO) Writing coverage report...

------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                       Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
src/gbn_protocol.c                           173      173   100%
src/packets.c                                 54       54   100%
src/transfer_io.c                            289      289   100%
------------------------------------------------------------------------------
TOTAL                                        516      516   100%
------------------------------------------------------------------------------
```

---

## Address Sanitizer Report

This section was generated by running `make leak-test` in the project root directory.

```bash
Invalid sender configuration
Invalid receiver configuration
Invalid receiver configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Invalid sender configuration
Relay refused registration: test refusal
Relay registration timed out after five attempts
Transfer failed after ten timeouts without progress
Relay socket failed
getaddrinfo: Name or service not known
input file: No such file or directory
read input: Is a directory
Input exceeds 16 MiB
Relay refused registration: test refusal
getaddrinfo: Name or service not known
output file: No such file or directory
close output: No space left on device
Receiver idle timeout or input limit exceeded
tests/lab-test.c:665:test_checksum_examples:PASS
tests/lab-test.c:666:test_packet_worked_examples_and_round_trip:PASS
tests/lab-test.c:667:test_packet_invalid_arguments_and_fields:PASS
tests/lab-test.c:668:test_receiver_order_duplicates_gaps_and_finish:PASS
tests/lab-test.c:669:test_receiver_idle_and_packet_deadline:PASS
tests/lab-test.c:670:test_receiver_input_limit_and_saturated_clock:PASS
tests/lab-test.c:671:test_protocol_null_arguments:PASS
tests/lab-test.c:672:test_sender_window_acknowledgments_and_fin:PASS
tests/lab-test.c:673:test_sender_empty_partial_and_window_limits:PASS
tests/lab-test.c:674:test_sender_failure_and_progress_reset:PASS
tests/lab-test.c:675:test_lost_finish_acknowledgment_recovery:PASS
tests/lab-test.c:676:test_seeded_damaged_transfers:PASS
Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]
                  [-c corrupt] [-d dup] [-p port] <relay> <file>
       myapp recv -s <session> [-p port] <relay> <file>
  -s <session>     1 to 32 characters from a-z, 0-9 and -
  -w <window>      window in packets, 1 to 64 (default: 8)
  -T <timeout-ms>  positive retransmission timeout (default: 250)
  -l <loss>        drop probability, 0 to 0.5 (default: 0)
  -c <corrupt>     corruption probability, 0 to 0.5 (default: 0)
  -d <dup>         duplication probability, 0 to 0.5 (default: 0)
  -p <port>        relay port, 1 to 65535 (default: 4250)
tests/lab-test.c:514:test_cli_defaults_and_options:PASS
tests/lab-test.c:515:test_cli_invalid_arguments:PASS
tests/lab-test.c:516:test_io_invalid_configuration:PASS
tests/lab-test.c:517:test_io_sender_success_retries_and_failures:PASS
tests/lab-test.c:518:test_io_receiver_success_and_failures:PASS

-----------------------
17 Tests 0 Failures 0 Ignored 
OK
```

---

## Src Files
### gbn_protocol.c

```c

#include "gbn_protocol.h"
#include <stdlib.h>
#include <string.h>

/* #region Constants */
/* #endregion Constants */
/* #region Types */
struct receiver_protocol_state
{
    uint32_t expected_sequence_number;
    size_t delivered_size;
    uint64_t deadline_ms;
    bool lingering;
    protocol_status status;
};

struct sender_protocol_state
{
    uint8_t *input;
    size_t input_size;
    size_t window_size;
    uint32_t timeout_ms;
    uint32_t data_packet_count;
    uint32_t base_sequence_number;
    uint32_t next_sequence_number;
    unsigned fruitless_timeouts;
    uint64_t deadline_ms;
    bool started;
    protocol_status status;
    encoded_protocol_packet window[PROTOCOL_MAX_WINDOW];
};
/* #endregion Types */

/* #region Helpers */
/* Initialize caller-owned action storage, defaulting to failure for invalid events. */
static void initialize_protocol_actions(protocol_actions *actions)
{
    memset(actions, 0, sizeof(*actions));
    actions->status = PROTOCOL_FAILED;
    actions->deadline_ms = PROTOCOL_NO_DEADLINE;
}

/* Saturate synthetic timestamps without overflowing or using the no-timer sentinel. */
static uint64_t calculate_protocol_deadline(uint64_t now_ms, uint32_t duration_ms)
{
    if (now_ms >= UINT64_MAX - duration_ms)
    {
        return UINT64_MAX - 1;
    }
    return now_ms + duration_ms;
}
/* #endregion Helpers */

/* #region Functions */
receiver_protocol_state *create_receiver_protocol_state(uint64_t now_ms)
{
    receiver_protocol_state *state = calloc(1, sizeof(*state));
    if (state == NULL) // GCOVR_EXCL_START
    {
        return NULL;
    } // GCOVR_EXCL_STOP
    state->deadline_ms = calculate_protocol_deadline(now_ms, PROTOCOL_RECEIVER_IDLE_MS);
    return state;
}

void destroy_receiver_protocol_state(receiver_protocol_state *state)
{
    free(state);
}
/* #endregion Functions */

/* #region Helpers */
/* Construct the cumulative ACK in caller-owned storage; fields are valid by construction. */
static void append_receiver_acknowledgment(
    const receiver_protocol_state *state,
    protocol_actions *actions)
{
    protocol_packet acknowledgment = {.type = PROTOCOL_ACK,
        .sequence_number = state->expected_sequence_number};
    (void)encode_protocol_packet(&acknowledgment, &actions->packets[0]);
    actions->packet_count = 1;
}

/* Deliver expected DATA exactly once, enforcing the assignment's total file limit. */
static void process_receiver_data(
    receiver_protocol_state *state,
    const protocol_packet *packet,
    protocol_actions *actions)
{
    if (!state->lingering && packet->sequence_number == state->expected_sequence_number)
    {
        if (packet->payload_length > PROTOCOL_MAX_INPUT_SIZE - state->delivered_size ||
            state->expected_sequence_number >= PROTOCOL_MAX_INPUT_SIZE / PROTOCOL_PAYLOAD_SIZE)
        {
            state->status = PROTOCOL_FAILED;
            return;
        }
        memcpy(actions->delivery, packet->payload, packet->payload_length);
        actions->delivery_size = packet->payload_length;
        state->delivered_size += packet->payload_length;
        state->expected_sequence_number++;
    }
    append_receiver_acknowledgment(state, actions);
}

/* Accept only the expected FIN; duplicates and premature FINs repeat the current ACK. */
static void process_receiver_finish(
    receiver_protocol_state *state,
    const protocol_packet *packet,
    uint64_t now_ms,
    protocol_actions *actions)
{
    if (!state->lingering && packet->sequence_number == state->expected_sequence_number)
    {
        state->expected_sequence_number++;
        state->lingering = true;
        state->deadline_ms = calculate_protocol_deadline(now_ms, PROTOCOL_RECEIVER_LINGER_MS);
        actions->close_output = true;
    }
    append_receiver_acknowledgment(state, actions);
}

/* Expire before processing events so continuous traffic cannot extend a fixed linger. */
static void update_receiver_deadline(receiver_protocol_state *state, uint64_t now_ms)
{
    if (state->status == PROTOCOL_ACTIVE && now_ms >= state->deadline_ms)
    {
        state->status = state->lingering ? PROTOCOL_COMPLETE : PROTOCOL_FAILED;
    }
}

/* Report terminal state without scheduling further wakeups. */
static void report_receiver_state(
    const receiver_protocol_state *state,
    protocol_actions *actions)
{
    actions->status = state->status;
    actions->deadline_ms = state->status == PROTOCOL_ACTIVE ?
        state->deadline_ms : PROTOCOL_NO_DEADLINE;
}
/* #endregion Helpers */

/* #region Functions */
void process_receiver_protocol_packet(
    receiver_protocol_state *state,
    const protocol_packet *packet,
    uint64_t now_ms,
    protocol_actions *actions)
{
    if (actions == NULL)
    {
        return;
    }
    initialize_protocol_actions(actions);
    if (state == NULL || packet == NULL)
    {
        return;
    }
    update_receiver_deadline(state, now_ms);
    if (state->status == PROTOCOL_ACTIVE)
    {
        if (!state->lingering)
        {
            state->deadline_ms = calculate_protocol_deadline(now_ms, PROTOCOL_RECEIVER_IDLE_MS);
        }
        if (packet->type == PROTOCOL_DATA)
        {
            process_receiver_data(state, packet, actions);
        }
        else if (packet->type == PROTOCOL_FIN)
        {
            process_receiver_finish(state, packet, now_ms, actions);
        }
    }
    report_receiver_state(state, actions);
}

void process_receiver_protocol_timeout(
    receiver_protocol_state *state,
    uint64_t now_ms,
    protocol_actions *actions)
{
    if (actions == NULL)
    {
        return;
    }
    initialize_protocol_actions(actions);
    if (state == NULL)
    {
        return;
    }
    update_receiver_deadline(state, now_ms);
    report_receiver_state(state, actions);
}

sender_protocol_state *create_sender_protocol_state(
    const uint8_t *input,
    size_t input_size,
    size_t window_size,
    uint32_t timeout_ms)
{
    if ((input == NULL && input_size != 0) || input_size > PROTOCOL_MAX_INPUT_SIZE ||
        window_size == 0 || window_size > PROTOCOL_MAX_WINDOW || timeout_ms == 0)
    {
        return NULL;
    }
    sender_protocol_state *state = calloc(1, sizeof(*state));
    if (state == NULL) // GCOVR_EXCL_START
    {
        return NULL;
    } // GCOVR_EXCL_STOP
    if (input_size != 0)
    {
        state->input = malloc(input_size);
        if (state->input == NULL) // GCOVR_EXCL_START
        {
            free(state);
            return NULL;
        } // GCOVR_EXCL_STOP
        memcpy(state->input, input, input_size);
    }
    state->input_size = input_size;
    state->window_size = window_size;
    state->timeout_ms = timeout_ms;
    state->data_packet_count = (uint32_t)((input_size + PROTOCOL_PAYLOAD_SIZE - 1) /
                                         PROTOCOL_PAYLOAD_SIZE);
    state->deadline_ms = PROTOCOL_NO_DEADLINE;
    return state;
}

void destroy_sender_protocol_state(sender_protocol_state *state)
{
    if (state != NULL)
    {
        free(state->input);
        free(state);
    }
}
/* #endregion Functions */

/* #region Helpers */
/* Construct and retain a packet in its ring slot until cumulative acknowledgment. */
static void construct_sender_packet(sender_protocol_state *state, uint32_t sequence_number)
{
    protocol_packet packet = {.sequence_number = sequence_number};
    if (sequence_number == state->data_packet_count)
    {
        packet.type = PROTOCOL_FIN;
    }
    else
    {
        size_t offset = (size_t)sequence_number * PROTOCOL_PAYLOAD_SIZE;
        size_t remaining = state->input_size - offset;
        packet.payload_length = (uint16_t)(remaining < PROTOCOL_PAYLOAD_SIZE ?
                                          remaining : PROTOCOL_PAYLOAD_SIZE);
        memcpy(packet.payload, state->input + offset, packet.payload_length);
    }
    (void)encode_protocol_packet(&packet, &state->window[sequence_number % state->window_size]);
}

/* Copy a retained datagram into independent caller-owned action storage. */
static void append_sender_packet(
    const sender_protocol_state *state,
    uint32_t sequence_number,
    protocol_actions *actions)
{
    actions->packets[actions->packet_count++] =
        state->window[sequence_number % state->window_size];
}

/* Fill only DATA slots; FIN waits until every DATA has been acknowledged. */
static void fill_sender_window(sender_protocol_state *state, protocol_actions *actions)
{
    while (state->next_sequence_number < state->data_packet_count &&
           state->next_sequence_number - state->base_sequence_number < state->window_size)
    {
        construct_sender_packet(state, state->next_sequence_number);
        append_sender_packet(state, state->next_sequence_number, actions);
        state->next_sequence_number++;
    }
    /* This helper runs initially or after advancing base. At the DATA boundary,
     * next is the same index and FIN has never been sent. Duplicate ACKs do not
     * enter this helper, so FIN cannot be constructed a second time. */
    if (state->base_sequence_number == state->data_packet_count)
    {
        construct_sender_packet(state, state->next_sequence_number);
        append_sender_packet(state, state->next_sequence_number, actions);
        state->next_sequence_number++;
    }
}

/* One timer tracks all outstanding packets; stop it when the window is empty. */
static void restart_sender_timer(sender_protocol_state *state, uint64_t now_ms)
{
    state->deadline_ms = state->base_sequence_number < state->next_sequence_number ?
        calculate_protocol_deadline(now_ms, state->timeout_ms) : PROTOCOL_NO_DEADLINE;
}

/* Validate ACK progress against the transmitted window, reset failures, then refill. */
static void advance_sender_acknowledgment(
    sender_protocol_state *state,
    uint32_t acknowledgment,
    uint64_t now_ms,
    protocol_actions *actions)
{
    state->base_sequence_number = acknowledgment;
    state->fruitless_timeouts = 0;
    if (acknowledgment == state->data_packet_count + 1)
    {
        state->status = PROTOCOL_COMPLETE;
    }
    else
    {
        fill_sender_window(state, actions);
    }
    restart_sender_timer(state, now_ms);
}

/* Report the timer/status after an event; terminal states have no timer. */
static void report_sender_state(const sender_protocol_state *state, protocol_actions *actions)
{
    actions->status = state->status;
    actions->deadline_ms = state->status == PROTOCOL_ACTIVE ?
        state->deadline_ms : PROTOCOL_NO_DEADLINE;
}
/* #endregion Helpers */

/* #region Functions */
void start_sender_protocol_transfer(
    sender_protocol_state *state,
    uint64_t now_ms,
    protocol_actions *actions)
{
    if (actions == NULL)
    {
        return;
    }
    initialize_protocol_actions(actions);
    if (state == NULL)
    {
        return;
    }
    if (!state->started)
    {
        state->started = true;
        fill_sender_window(state, actions);
        restart_sender_timer(state, now_ms);
    }
    report_sender_state(state, actions);
}

void process_sender_protocol_acknowledgment(
    sender_protocol_state *state,
    const protocol_packet *packet,
    uint64_t now_ms,
    protocol_actions *actions)
{
    if (actions == NULL)
    {
        return;
    }
    initialize_protocol_actions(actions);
    if (state == NULL || packet == NULL)
    {
        return;
    }
    if (state->started && state->status == PROTOCOL_ACTIVE && packet->type == PROTOCOL_ACK &&
        packet->sequence_number > state->base_sequence_number &&
        packet->sequence_number <= state->next_sequence_number)
    {
        advance_sender_acknowledgment(state, packet->sequence_number, now_ms, actions);
    }
    report_sender_state(state, actions);
}

void process_sender_protocol_timeout(
    sender_protocol_state *state,
    uint64_t now_ms,
    protocol_actions *actions)
{
    if (actions == NULL)
    {
        return;
    }
    initialize_protocol_actions(actions);
    if (state == NULL)
    {
        return;
    }
    if (state->status == PROTOCOL_ACTIVE && state->deadline_ms != PROTOCOL_NO_DEADLINE &&
        now_ms >= state->deadline_ms)
    {
        state->fruitless_timeouts++;
        if (state->fruitless_timeouts >= PROTOCOL_MAX_TIMEOUTS)
        {
            state->status = PROTOCOL_FAILED;
            state->deadline_ms = PROTOCOL_NO_DEADLINE;
        }
        else
        {
            for (uint32_t sequence_number = state->base_sequence_number;
                 sequence_number < state->next_sequence_number; sequence_number++)
            {
                append_sender_packet(state, sequence_number, actions);
            }
            restart_sender_timer(state, now_ms);
        }
    }
    report_sender_state(state, actions);
}
/* #endregion Functions */

```

### gbn_protocol.h

```c

#ifndef GBN_PROTOCOL_H
#define GBN_PROTOCOL_H

#include "packets.h"

/* #region Constants */
#define PROTOCOL_MAX_WINDOW 64U
#define PROTOCOL_MAX_INPUT_SIZE (16U * 1024U * 1024U)
#define PROTOCOL_MAX_TIMEOUTS 10U
#define PROTOCOL_RECEIVER_IDLE_MS 30000U
#define PROTOCOL_RECEIVER_LINGER_MS 2000U
#define PROTOCOL_NO_DEADLINE UINT64_MAX
/* #endregion Constants */

/* #region Types */
typedef struct sender_protocol_state sender_protocol_state;
typedef struct receiver_protocol_state receiver_protocol_state;
typedef enum
{
    PROTOCOL_ACTIVE,
    PROTOCOL_COMPLETE,
    PROTOCOL_FAILED
} protocol_status;
typedef struct
{
    protocol_status status;
    uint64_t deadline_ms;
    size_t packet_count;
    encoded_protocol_packet packets[PROTOCOL_MAX_WINDOW];
    size_t delivery_size;
    uint8_t delivery[PROTOCOL_PAYLOAD_SIZE];
    bool close_output;
} protocol_actions;
/* #endregion Types */

/* #region Functions */
/** Parameters: now_ms is the initial monotonic timestamp in milliseconds.
 *  Returns: an owned receiver state, or NULL on allocation failure. Notes:
 *  starts expecting sequence zero with a 30-second idle deadline; release with
 *  destroy_receiver_protocol_state(). No I/O occurs. */
receiver_protocol_state *create_receiver_protocol_state(uint64_t now_ms);

/** Parameters: state is an owned receiver state or NULL. Returns: nothing.
 *  Notes: releases all state; NULL is harmless. Never reuse a destroyed state. */
void destroy_receiver_protocol_state(receiver_protocol_state *state);

/** Parameters: state is live; packet is a borrowed validated packet; now_ms is
 *  nondecreasing; actions is caller-owned fixed writable storage. Returns:
 *  nothing; actions describes ACKs, delivered bytes, file closure, status, and
 *  deadline. Notes: NULL state/packet yields failure; NULL actions does nothing.
 *  Buffers own their bytes and survive subsequent events. Only in-order DATA
 *  is delivered; FIN starts a fixed two-second linger. Execute actions before
 *  another event. The state machine never accesses sockets, clocks or files. */
void process_receiver_protocol_packet(
    receiver_protocol_state *state,
    const protocol_packet *packet,
    uint64_t now_ms,
    protocol_actions *actions);

/** Parameters: state is live; now_ms is nondecreasing; actions is writable.
 *  Returns: nothing; actions contains status/deadline and no transmissions.
 *  Notes: early events do nothing, idle expiry fails, linger expiry succeeds.
 *  NULL state yields failure; NULL actions does nothing. No I/O occurs. */
void process_receiver_protocol_timeout(
    receiver_protocol_state *state,
    uint64_t now_ms,
    protocol_actions *actions);

/** Parameters: input is borrowed input_size bytes (NULL only for empty input);
 *  input_size is at most 16 MiB; window_size is 1..64; timeout_ms is positive.
 *  Returns: an owned state, or NULL for invalid parameters/allocation failure.
 *  Notes: owns a copy of input, which the caller may immediately free; release
 *  with destroy_sender_protocol_state(). Transfer has not started. */
sender_protocol_state *create_sender_protocol_state(
    const uint8_t *input,
    size_t input_size,
    size_t window_size,
    uint32_t timeout_ms);

/** Parameters: state is an owned sender state or NULL. Returns: nothing.
 *  Notes: frees the input copy and retransmission storage; NULL is harmless. */
void destroy_sender_protocol_state(sender_protocol_state *state);

/** Parameters: state is live; now_ms is monotonic; actions is caller-owned
 *  writable fixed storage. Returns: nothing; actions contains the initial
 *  window and timer deadline. Notes: starts once; repeated calls report current
 *  state without resending. Empty input sends FIN zero. NULL state yields
 *  failure; NULL actions does nothing. Execute actions before another event. */
void start_sender_protocol_transfer(
    sender_protocol_state *state,
    uint64_t now_ms,
    protocol_actions *actions);

/** Parameters: state is live; packet is a borrowed validated packet; now_ms is
 *  nondecreasing; actions is caller-owned fixed writable storage. Returns:
 *  nothing; actions contains newly opened window packets and status/deadline.
 *  Notes: ignores non-ACKs, duplicates and ACKs beyond sent packets. Progress
 *  resets the fruitless-timeout counter and restarts the timer. FIN is sent
 *  only after all DATA is acknowledged. NULL state/packet yields failure; NULL
 *  actions does nothing. Buffers own their bytes; no I/O occurs. */
void process_sender_protocol_acknowledgment(
    sender_protocol_state *state,
    const protocol_packet *packet,
    uint64_t now_ms,
    protocol_actions *actions);

/** Parameters: state is live; now_ms is nondecreasing; actions is writable.
 *  Returns: nothing; actions contains the whole outstanding window on expiry,
 *  or failure at the tenth consecutive fruitless timeout. Notes: early events
 *  do nothing. NULL state yields failure; NULL actions does nothing. Returned
 *  buffers own their bytes; no socket, clock, or file is accessed. */
void process_sender_protocol_timeout(
    sender_protocol_state *state,
    uint64_t now_ms,
    protocol_actions *actions);
/* #endregion Functions */
#endif

```

### lab.c

```c

#include "lab.h"

/* The umbrella's implementation lives in packets.c, gbn_protocol.c and transfer_io.c. */
/* #region Constants */
/* #endregion Constants */
/* #region Types */
/* #endregion Types */
/* #region Helpers */
/* #endregion Helpers */
/* #region Functions */
/* #endregion Functions */

```

### lab.h

```c

#ifndef LAB_H
#define LAB_H

/* #region Constants */
/* #endregion Constants */
/* #region Types */
#include "packets.h"
#include "gbn_protocol.h"
#include "transfer_io.h"
/* #endregion Types */
/* #region Functions */
/* Public interfaces are documented in the layer headers. */
/* #endregion Functions */
#endif

```

### main.c

```c

#include "lab.h"

/* #region Constants */
/* #endregion Constants */
/* #region Types */
/* #endregion Types */
/* #region Helpers */
/* #endregion Helpers */
/* #region Functions */
#ifdef TEST
#define main main_exclude
#endif

int main(int argc, char *argv[])
{
    transfer_configuration configuration;
    transfer_parse_result result = parse_transfer_configuration(argc, argv, &configuration);
    if (result != TRANSFER_PARSE_OK)
    {
        print_transfer_usage();
        return result == TRANSFER_PARSE_USAGE ? 0 : 1;
    }
    if (configuration.mode == TRANSFER_SEND)
    {
        return run_sender_transfer(&configuration);
    }
    return run_receiver_transfer(&configuration);
}
/* #endregion Functions */

```

### packets.c

```c

#include "packets.h"
#include <string.h>

/* #region Constants */
/* #endregion Constants */
/* #region Types */
/* #endregion Types */
/* #region Functions */
uint16_t calculate_internet_checksum(const uint8_t *bytes, size_t size)
{
    if (bytes == NULL && size != 0)
    {
        return 0;
    }
    uint32_t sum = 0;
    for (size_t index = 0; index < size; index += 2)
    {
        sum += (uint32_t)bytes[index] << 8;
        if (index + 1 < size)
        {
            sum += bytes[index + 1];
        }
        sum = (sum & UINT16_MAX) + (sum >> 16);
    }
    sum = (sum & UINT16_MAX) + (sum >> 16);
    return (uint16_t)~sum;
}
/* #endregion Functions */

/* #region Helpers */
/* Read/write unsigned header fields without alignment or struct-padding assumptions.
 * Buffers are borrowed and must contain the requested number of bytes. */
static uint16_t read_header_uint16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] << 8 | bytes[1]);
}

static uint32_t read_header_uint32(const uint8_t *bytes)
{
    return (uint32_t)read_header_uint16(bytes) << 16 | read_header_uint16(bytes + 2);
}

static void write_header_uint16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}

static void write_header_uint32(uint8_t *bytes, uint32_t value)
{
    write_header_uint16(bytes, (uint16_t)(value >> 16));
    write_header_uint16(bytes + 2, (uint16_t)value);
}
/* #endregion Helpers */

/* #region Functions */
bool encode_protocol_packet(
    const protocol_packet *packet,
    encoded_protocol_packet *output)
{
    if (packet == NULL || output == NULL ||
        packet->type < PROTOCOL_DATA || packet->type > PROTOCOL_FIN ||
        packet->payload_length > PROTOCOL_PAYLOAD_SIZE ||
        (packet->type != PROTOCOL_DATA && packet->payload_length != 0))
    {
        return false;
    }
    output->size = PROTOCOL_HEADER_SIZE + packet->payload_length;
    output->bytes[0] = (uint8_t)packet->type;
    output->bytes[1] = 0;
    write_header_uint16(output->bytes + 2, 0);
    write_header_uint32(output->bytes + 4, packet->sequence_number);
    write_header_uint16(output->bytes + 8, packet->payload_length);
    memcpy(output->bytes + PROTOCOL_HEADER_SIZE, packet->payload, packet->payload_length);
    write_header_uint16(output->bytes + 2,
                        calculate_internet_checksum(output->bytes, output->size));
    return true;
}

bool decode_and_validate_protocol_packet(
    const uint8_t *bytes,
    size_t size,
    protocol_packet *output)
{
    if (bytes == NULL || output == NULL || size < PROTOCOL_HEADER_SIZE ||
        size > PROTOCOL_PACKET_SIZE)
    {
        return false;
    }
    uint16_t payload_length = read_header_uint16(bytes + 8);
    if (payload_length > PROTOCOL_PAYLOAD_SIZE ||
        PROTOCOL_HEADER_SIZE + payload_length != size ||
        bytes[0] > PROTOCOL_FIN || bytes[1] != 0 ||
        (bytes[0] != PROTOCOL_DATA && payload_length != 0) ||
        calculate_internet_checksum(bytes, size) != 0)
    {
        return false;
    }
    output->type = (protocol_packet_type)bytes[0];
    output->sequence_number = read_header_uint32(bytes + 4);
    output->payload_length = payload_length;
    memcpy(output->payload, bytes + PROTOCOL_HEADER_SIZE, payload_length);
    return true;
}
/* #endregion Functions */

```

### packets.h

```c

#ifndef PACKETS_H
#define PACKETS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* #region Constants */
#define PROTOCOL_HEADER_SIZE 10U
#define PROTOCOL_PAYLOAD_SIZE 1024U
#define PROTOCOL_PACKET_SIZE (PROTOCOL_HEADER_SIZE + PROTOCOL_PAYLOAD_SIZE)
/* #endregion Constants */

/* #region Types */
typedef enum
{
    PROTOCOL_DATA = 0,
    PROTOCOL_ACK = 1,
    PROTOCOL_FIN = 2
} protocol_packet_type;

typedef struct
{
    protocol_packet_type type;
    uint32_t sequence_number;
    uint16_t payload_length;
    uint8_t payload[PROTOCOL_PAYLOAD_SIZE];
} protocol_packet;

typedef struct
{
    size_t size;
    uint8_t bytes[PROTOCOL_PACKET_SIZE];
} encoded_protocol_packet;
/* #endregion Types */

/* #region Functions */
/** Parameters: bytes is a borrowed buffer of size bytes; NULL is allowed only
 *  when size is zero. Returns: the RFC 1071 checksum in host byte order, or
 *  zero for an invalid NULL buffer. Notes: odd lengths are logically padded;
 *  no allocation or modification occurs. A valid encoded packet returns zero. */
uint16_t calculate_internet_checksum(const uint8_t *bytes, size_t size);

/** Parameters: packet is borrowed; output points to writable fixed storage.
 *  Returns: true on success, false for NULL, invalid type/length, or a nonempty
 *  ACK/FIN. Notes: output is unchanged on failure; caller owns both objects.
 *  Every field is serialized explicitly in network byte order. */
bool encode_protocol_packet(
    const protocol_packet *packet,
    encoded_protocol_packet *output);

/** Parameters: bytes is a borrowed datagram of exactly size bytes; output is
 *  caller-owned writable packet storage. Returns: true for a valid packet,
 *  false for NULL, malformed fields/size, or checksum corruption. Notes:
 *  output is unchanged on failure; payload length is checked before copying;
 *  ACK/FIN must have zero payload. No allocation or cleanup is needed. */
bool decode_and_validate_protocol_packet(
    const uint8_t *bytes,
    size_t size,
    protocol_packet *output);
/* #endregion Functions */
#endif

```

### transfer_io.c

```c

#define _POSIX_C_SOURCE 200809L
#include "transfer_io.h"
#include "gbn_protocol.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
extern int optreset;
#endif

/* #region Constants */
#define REGISTRATION_ATTEMPTS 5U
#define REGISTRATION_TIMEOUT_MS 1000U
/* #endregion Constants */

/* #region Types */
typedef struct
{
    int socket_descriptor;
    FILE *file;
    uint8_t *input;
    sender_protocol_state *sender;
    receiver_protocol_state *receiver;
} transfer_resources;
/* #endregion Types */

/* #region Helpers */
/* Strict decimal integers: reject signs, whitespace, overflow and trailing characters. */
static bool parse_unsigned_value(const char *text, uint32_t maximum, uint32_t *value)
{
    if (*text == '\0')
    {
        return false;
    }
    uint32_t parsed = 0;
    for (const char *character = text; *character != '\0'; character++)
    {
        if (*character < '0' || *character > '9')
        {
            return false;
        }
        uint32_t digit = (uint32_t)(*character - '0');
        if (parsed > maximum / 10 || (parsed == maximum / 10 && digit > maximum % 10))
        {
            return false;
        }
        parsed = parsed * 10 + digit;
    }
    if (parsed == 0)
    {
        return false;
    }
    *value = parsed;
    return true;
}

/* Parse the relay's decimal probability syntax, excluding NaN, signs and exponents. */
static bool parse_probability(const char *text, double *value)
{
    bool has_digit = false;
    bool has_point = false;
    for (const char *character = text; *character != '\0'; character++)
    {
        if (*character >= '0' && *character <= '9')
        {
            has_digit = true;
        }
        else if (*character == '.' && !has_point)
        {
            has_point = true;
        }
        else
        {
            return false;
        }
    }
    if (!has_digit || text[strlen(text) - 1] == '.')
    {
        return false;
    }
    errno = 0;
    /* Syntax above guarantees a complete, nonnegative decimal. ERANGE catches
     * overflow/underflow; NaN and infinity have already been rejected as text. */
    double parsed = strtod(text, NULL);
    if (errno != 0 || parsed > 0.5)
    {
        return false;
    }
    *value = parsed;
    return true;
}

/* Validate the session without locale-dependent character classification. */
static bool validate_session_name(const char *session)
{
    size_t length = strlen(session);
    if (length == 0 || length >= TRANSFER_SESSION_CAPACITY)
    {
        return false;
    }
    for (size_t index = 0; index < length; index++)
    {
        char character = session[index];
        if (!((character >= 'a' && character <= 'z') ||
              (character >= '0' && character <= '9') || character == '-'))
        {
            return false;
        }
    }
    return true;
}

/* Validate a public entry point's borrowed configuration before touching resources. */
static bool validate_transfer_configuration(
    const transfer_configuration *configuration,
    transfer_mode mode)
{
    return configuration != NULL && configuration->mode == mode &&
        memchr(configuration->session, '\0', TRANSFER_SESSION_CAPACITY) != NULL &&
        validate_session_name(configuration->session) && configuration->port != 0 &&
        configuration->relay_host != NULL && configuration->file_path != NULL &&
        configuration->window_size >= 1 && configuration->window_size <= PROTOCOL_MAX_WINDOW &&
        configuration->timeout_ms != 0 &&
        isfinite(configuration->loss_probability) && configuration->loss_probability >= 0 &&
        configuration->loss_probability <= 0.5 &&
        isfinite(configuration->corruption_probability) && configuration->corruption_probability >= 0 &&
        configuration->corruption_probability <= 0.5 &&
        isfinite(configuration->duplication_probability) && configuration->duplication_probability >= 0 &&
        configuration->duplication_probability <= 0.5;
}
/* #endregion Helpers */

/* #region Functions */
transfer_parse_result parse_transfer_configuration(
    int argc,
    char *argv[],
    transfer_configuration *configuration)
{
    if (argc < 1 || argv == NULL || configuration == NULL)
    {
        return TRANSFER_PARSE_ERROR;
    }
    if (argc == 1)
    {
        return TRANSFER_PARSE_USAGE;
    }
    transfer_configuration parsed = {
        .window_size = TRANSFER_DEFAULT_WINDOW,
        .timeout_ms = TRANSFER_DEFAULT_TIMEOUT_MS,
        .port = TRANSFER_DEFAULT_PORT
    };
    if (strcmp(argv[1], "send") == 0)
    {
        parsed.mode = TRANSFER_SEND;
    }
    else if (strcmp(argv[1], "recv") == 0)
    {
        parsed.mode = TRANSFER_RECEIVE;
    }
    else
    {
        return TRANSFER_PARSE_ERROR;
    }
    /* A fresh getopt argument vector begins at the mode, allowing repeat parsing in tests. */
#ifdef __APPLE__
    optreset = 1;
    optind = 1;
#else
    optind = 0;
#endif
    opterr = 0;
    int option;
    while ((option = getopt(argc - 1, argv + 1, "+s:w:T:l:c:d:p:")) != -1)
    {
        uint32_t numeric_value;
        if (parsed.mode == TRANSFER_RECEIVE && option != 's' && option != 'p')
        {
            return TRANSFER_PARSE_ERROR;
        }
        switch (option)
        {
            case 's':
                if (!validate_session_name(optarg))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                strcpy(parsed.session, optarg);
                break;
            case 'w':
                if (!parse_unsigned_value(optarg, PROTOCOL_MAX_WINDOW, &numeric_value))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                parsed.window_size = numeric_value;
                break;
            case 'T':
                if (!parse_unsigned_value(optarg, UINT32_MAX, &parsed.timeout_ms))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                break;
            case 'p':
                if (!parse_unsigned_value(optarg, UINT16_MAX, &numeric_value))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                parsed.port = (uint16_t)numeric_value;
                break;
            case 'l':
                if (!parse_probability(optarg, &parsed.loss_probability))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                break;
            case 'c':
                if (!parse_probability(optarg, &parsed.corruption_probability))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                break;
            case 'd':
                if (!parse_probability(optarg, &parsed.duplication_probability))
                {
                    return TRANSFER_PARSE_ERROR;
                }
                break;
            default:
                return TRANSFER_PARSE_ERROR;
        }
    }
    int positional_index = optind + 1;
    if (parsed.session[0] == '\0' || argc - positional_index != 2)
    {
        return TRANSFER_PARSE_ERROR;
    }
    parsed.relay_host = argv[positional_index];
    parsed.file_path = argv[positional_index + 1];
    *configuration = parsed;
    return TRANSFER_PARSE_OK;
}

void print_transfer_usage(void)
{
    puts("Usage: myapp send -s <session> [-w window] [-T timeout-ms] [-l loss]\n"
         "                  [-c corrupt] [-d dup] [-p port] <relay> <file>\n"
         "       myapp recv -s <session> [-p port] <relay> <file>\n"
         "  -s <session>     1 to 32 characters from a-z, 0-9 and -\n"
         "  -w <window>      window in packets, 1 to 64 (default: 8)\n"
         "  -T <timeout-ms>  positive retransmission timeout (default: 250)\n"
         "  -l <loss>        drop probability, 0 to 0.5 (default: 0)\n"
         "  -c <corrupt>     corruption probability, 0 to 0.5 (default: 0)\n"
         "  -d <dup>         duplication probability, 0 to 0.5 (default: 0)\n"
         "  -p <port>        relay port, 1 to 65535 (default: 4250)");
}
/* #endregion Functions */

/* #region Helpers */
/* Read the monotonic clock; caller provides storage, false indicates a clock failure. */
static bool read_monotonic_timestamp(uint64_t *now_ms)
{
    struct timespec timestamp;
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0) // GCOVR_EXCL_START
    {
        perror("clock_gettime");
        return false;
    } // GCOVR_EXCL_STOP
    *now_ms = (uint64_t)timestamp.tv_sec * 1000 + (uint64_t)timestamp.tv_nsec / 1000000;
    return true;
}

/* Poll one socket until an absolute deadline, retrying interrupts with time recomputed.
 * Returns 1 for readable data, 0 for deadline, -1 for network/clock failure. */
static int wait_for_datagram(int socket_descriptor, uint64_t deadline_ms)
{
    for (;;)
    {
        uint64_t now_ms;
        if (!read_monotonic_timestamp(&now_ms)) // GCOVR_EXCL_START
        {
            return -1;
        } // GCOVR_EXCL_STOP
        if (now_ms >= deadline_ms)
        {
            return 0;
        }
        uint64_t remaining_ms = deadline_ms - now_ms;
        int timeout_ms = remaining_ms > INT_MAX ? INT_MAX : (int)remaining_ms;
        struct pollfd descriptor = {.fd = socket_descriptor, .events = POLLIN};
        int result = poll(&descriptor, 1, timeout_ms);
        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            // GCOVR_EXCL_START
            perror("poll");
            return -1;
            // GCOVR_EXCL_STOP
        }
        if (result == 0)
        {
            continue;
        }
        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            fprintf(stderr, "Relay socket failed\n");
            return -1;
        }
        return 1;
    }
}

/* Send a borrowed complete datagram, retrying signals; caller owns socket and bytes. */
static bool transmit_datagram(int socket_descriptor, const uint8_t *bytes, size_t size)
{
    ssize_t transmitted;
    do
    {
        transmitted = send(socket_descriptor, bytes, size, 0);
    } while (transmitted < 0 && errno == EINTR);
    if (transmitted < 0 || (size_t)transmitted != size) // GCOVR_EXCL_START
    {
        perror("send");
        return false;
    } // GCOVR_EXCL_STOP
    return true;
}

/* Receive into caller-owned storage; an extra byte exposes oversized datagrams.
 * Returns datagram length, or -1 after reporting failure; retries signal interrupts. */
static ssize_t receive_datagram(int socket_descriptor, uint8_t *bytes, size_t capacity)
{
    ssize_t received;
    do
    {
        received = recv(socket_descriptor, bytes, capacity, 0);
    } while (received < 0 && errno == EINTR);
    if (received < 0) // GCOVR_EXCL_START
    {
        perror("recv");
    } // GCOVR_EXCL_STOP
    return received;
}

/* Resolve a borrowed host and connect UDP to filter traffic to the relay. Returns an
 * owned socket or -1; releases all address results and unsuccessful sockets. */
static int create_relay_socket(const transfer_configuration *configuration)
{
    char port[6];
    snprintf(port, sizeof(port), "%u", (unsigned)configuration->port);
    /* The supplied relay binds an IPv4 socket, including when named localhost. */
    struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_DGRAM};
    struct addrinfo *addresses = NULL;
    int error = getaddrinfo(configuration->relay_host, port, &hints, &addresses);
    if (error != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }
    int socket_descriptor = -1;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next)
    {
        socket_descriptor = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socket_descriptor < 0) // GCOVR_EXCL_START
        {
            continue;
        } // GCOVR_EXCL_STOP
        if (connect(socket_descriptor, address->ai_addr, address->ai_addrlen) == 0)
        {
            break;
        }
        // GCOVR_EXCL_START
        close(socket_descriptor);
        socket_descriptor = -1;
        // GCOVR_EXCL_STOP
    }
    freeaddrinfo(addresses);
    if (socket_descriptor < 0) // GCOVR_EXCL_START
    {
        perror("socket/connect");
    } // GCOVR_EXCL_STOP
    return socket_descriptor;
}

/* Register over the same socket used for transfer. Returns false on refusal/failure;
 * retries silence five times, one second each; ignores unrelated/malformed replies. */
static bool register_with_relay(
    int socket_descriptor,
    const transfer_configuration *configuration)
{
    char hello[160];
    int hello_size;
    if (configuration->mode == TRANSFER_SEND)
    {
        hello_size = snprintf(hello, sizeof(hello), "HELLO %s send %.17f %.17f %.17f",
                              configuration->session, configuration->loss_probability,
                              configuration->corruption_probability,
                              configuration->duplication_probability);
    }
    else
    {
        hello_size = snprintf(hello, sizeof(hello), "HELLO %s recv", configuration->session);
    }
    for (unsigned attempt = 0; attempt < REGISTRATION_ATTEMPTS; attempt++)
    {
        if (!transmit_datagram(socket_descriptor, (const uint8_t *)hello, (size_t)hello_size))
        {
            return false; // GCOVR_EXCL_LINE
        }
        uint64_t now_ms;
        if (!read_monotonic_timestamp(&now_ms)) // GCOVR_EXCL_START
        {
            return false;
        } // GCOVR_EXCL_STOP
        uint64_t deadline_ms = now_ms + REGISTRATION_TIMEOUT_MS;
        for (;;)
        {
            int ready = wait_for_datagram(socket_descriptor, deadline_ms);
            if (ready < 0)
            {
                return false;
            }
            if (ready == 0)
            {
                break;
            }
            uint8_t reply[PROTOCOL_PACKET_SIZE + 1];
            ssize_t reply_size = receive_datagram(socket_descriptor, reply, sizeof(reply) - 1);
            if (reply_size < 0) // GCOVR_EXCL_START
            {
                return false;
            } // GCOVR_EXCL_STOP
            if (reply_size == 2 && memcmp(reply, "OK", 2) == 0)
            {
                return true;
            }
            if (reply_size >= 4 && memcmp(reply, "ERR ", 4) == 0)
            {
                reply[reply_size] = '\0';
                fprintf(stderr, "Relay refused registration: %s\n", (char *)reply + 4);
                return false;
            }
        }
    }
    fprintf(stderr, "Relay registration timed out after five attempts\n");
    return false;
}

/* Create an owned resource bundle with a sentinel socket, initially no resources. */
static transfer_resources *create_transfer_resources(void)
{
    transfer_resources *resources = calloc(1, sizeof(*resources));
    if (resources != NULL)
    {
        resources->socket_descriptor = -1;
    }
    return resources;
}

/* Release all owned resources; NULL is harmless. Returns false if file close fails. */
static bool destroy_transfer_resources(transfer_resources *resources)
{
    bool success = true;
    if (resources != NULL)
    {
        if (resources->file != NULL && fclose(resources->file) != 0) // GCOVR_EXCL_BR_LINE
        {
            // GCOVR_EXCL_START
            perror("fclose");
            success = false;
            // GCOVR_EXCL_STOP
        }
        if (resources->socket_descriptor >= 0)
        {
            close(resources->socket_descriptor);
        }
        free(resources->input);
        destroy_sender_protocol_state(resources->sender);
        destroy_receiver_protocol_state(resources->receiver);
        free(resources);
    }
    return success;
}

/* Load at most 16 MiB plus a detection byte, supporting empty and nonseekable input.
 * The bundle owns both the allocation and file, including on failure. */
static bool load_transfer_input(
    transfer_resources *resources,
    const char *file_path,
    size_t *input_size)
{
    resources->file = fopen(file_path, "rb");
    if (resources->file == NULL)
    {
        perror("input file");
        return false;
    }
    resources->input = malloc(PROTOCOL_MAX_INPUT_SIZE + 1);
    if (resources->input == NULL) // GCOVR_EXCL_START
    {
        perror("input allocation");
        return false;
    } // GCOVR_EXCL_STOP
    *input_size = fread(resources->input, 1, PROTOCOL_MAX_INPUT_SIZE + 1, resources->file);
    if (ferror(resources->file))
    {
        perror("read input");
        return false;
    }
    if (*input_size > PROTOCOL_MAX_INPUT_SIZE)
    {
        fprintf(stderr, "Input exceeds 16 MiB\n");
        return false;
    }
    int close_result = fclose(resources->file);
    resources->file = NULL;
    if (close_result != 0) // GCOVR_EXCL_START
    {
        perror("close input");
        return false;
    } // GCOVR_EXCL_STOP
    return true;
}

/* Execute delivery, closure and packet sends in order. Returns false on any I/O error;
 * close before ACKing FIN so a failed flush cannot acknowledge successful delivery. */
static bool execute_protocol_actions(
    transfer_resources *resources,
    const protocol_actions *actions)
{
    if (actions->delivery_size != 0 &&
        fwrite(actions->delivery, 1, actions->delivery_size, resources->file) != actions->delivery_size)
    {
        // GCOVR_EXCL_START
        perror("write output");
        return false;
        // GCOVR_EXCL_STOP
    }
    if (actions->close_output)
    {
        int close_result = fclose(resources->file);
        resources->file = NULL;
        if (close_result != 0)
        {
            perror("close output");
            return false;
        }
    }
    for (size_t index = 0; index < actions->packet_count; index++)
    {
        if (!transmit_datagram(resources->socket_descriptor,
                               actions->packets[index].bytes, actions->packets[index].size))
        {
            return false; // GCOVR_EXCL_LINE
        }
    }
    return true;
}

/* Drive either pure state machine from poll events; malformed packets have no effect.
 * Caller owns bundle/actions; returns the assignment's success/failure exit status. */
static int run_transfer_loop(transfer_resources *resources, protocol_actions *actions)
{
    for (;;)
    {
        if (!execute_protocol_actions(resources, actions))
        {
            return 2;
        }
        if (actions->status == PROTOCOL_COMPLETE)
        {
            return 0;
        }
        if (actions->status == PROTOCOL_FAILED)
        {
            fprintf(stderr, "%s\n", resources->sender != NULL ?
                    "Transfer failed after ten timeouts without progress" :
                    "Receiver idle timeout or input limit exceeded");
            return 2;
        }
        int ready = wait_for_datagram(resources->socket_descriptor, actions->deadline_ms);
        if (ready < 0)
        {
            return 2;
        }
        uint64_t now_ms;
        if (!read_monotonic_timestamp(&now_ms)) // GCOVR_EXCL_START
        {
            return 2;
        } // GCOVR_EXCL_STOP
        if (ready == 0)
        {
            if (resources->sender != NULL)
            {
                process_sender_protocol_timeout(resources->sender, now_ms, actions);
            }
            else
            {
                process_receiver_protocol_timeout(resources->receiver, now_ms, actions);
            }
        }
        else
        {
            uint8_t datagram[PROTOCOL_PACKET_SIZE + 1];
            ssize_t datagram_size = receive_datagram(resources->socket_descriptor,
                                                    datagram, sizeof(datagram));
            if (datagram_size < 0) // GCOVR_EXCL_START
            {
                return 2;
            } // GCOVR_EXCL_STOP
            protocol_packet packet;
            if (decode_and_validate_protocol_packet(datagram, (size_t)datagram_size, &packet))
            {
                if (resources->sender != NULL)
                {
                    process_sender_protocol_acknowledgment(resources->sender, &packet, now_ms, actions);
                }
                else
                {
                    process_receiver_protocol_packet(resources->receiver, &packet, now_ms, actions);
                }
            }
            else
            {
                /* Preserve deadline/status but never repeat a previous event's actions. */
                actions->packet_count = 0;
                actions->delivery_size = 0;
                actions->close_output = false;
            }
        }
    }
}
/* #endregion Helpers */

/* #region Functions */
int run_sender_transfer(const transfer_configuration *configuration)
{
    if (!validate_transfer_configuration(configuration, TRANSFER_SEND))
    {
        fprintf(stderr, "Invalid sender configuration\n");
        return 2;
    }
    transfer_resources *resources = create_transfer_resources();
    if (resources == NULL) // GCOVR_EXCL_START
    {
        return 2;
    } // GCOVR_EXCL_STOP
    int exit_status = 2;
    size_t input_size;
    if (load_transfer_input(resources, configuration->file_path, &input_size))
    {
        resources->sender = create_sender_protocol_state(resources->input, input_size,
                                                        configuration->window_size,
                                                        configuration->timeout_ms);
        free(resources->input);
        resources->input = NULL;
        resources->socket_descriptor = create_relay_socket(configuration);
        uint64_t now_ms;
        if (resources->sender != NULL && resources->socket_descriptor >= 0 &&
            register_with_relay(resources->socket_descriptor, configuration) &&
            read_monotonic_timestamp(&now_ms))
        {
            protocol_actions actions;
            start_sender_protocol_transfer(resources->sender, now_ms, &actions);
            exit_status = run_transfer_loop(resources, &actions);
        }
    }
    if (!destroy_transfer_resources(resources))
    {
        exit_status = 2; // GCOVR_EXCL_LINE
    }
    return exit_status;
}

int run_receiver_transfer(const transfer_configuration *configuration)
{
    if (!validate_transfer_configuration(configuration, TRANSFER_RECEIVE))
    {
        fprintf(stderr, "Invalid receiver configuration\n");
        return 2;
    }
    transfer_resources *resources = create_transfer_resources();
    if (resources == NULL) // GCOVR_EXCL_START
    {
        return 2;
    } // GCOVR_EXCL_STOP
    int exit_status = 2;
    resources->socket_descriptor = create_relay_socket(configuration);
    if (resources->socket_descriptor >= 0 && register_with_relay(resources->socket_descriptor, configuration))
    {
        resources->file = fopen(configuration->file_path, "wb");
        if (resources->file == NULL)
        {
            perror("output file");
        }
        else
        {
            uint64_t now_ms;
            if (read_monotonic_timestamp(&now_ms))
            {
                resources->receiver = create_receiver_protocol_state(now_ms);
                protocol_actions actions;
                process_receiver_protocol_timeout(resources->receiver, now_ms, &actions);
                exit_status = run_transfer_loop(resources, &actions);
            }
        }
    }
    if (!destroy_transfer_resources(resources))
    {
        exit_status = 2; // GCOVR_EXCL_LINE
    }
    return exit_status;
}
/* #endregion Functions */

```

### transfer_io.h

```c

#ifndef TRANSFER_IO_H
#define TRANSFER_IO_H

#include <stdint.h>

/* #region Constants */
#define TRANSFER_DEFAULT_PORT 4250U
#define TRANSFER_DEFAULT_WINDOW 8U
#define TRANSFER_DEFAULT_TIMEOUT_MS 250U
#define TRANSFER_SESSION_CAPACITY 33U
/* #endregion Constants */

/* #region Types */
typedef enum
{
    TRANSFER_SEND,
    TRANSFER_RECEIVE
} transfer_mode;
typedef struct
{
    transfer_mode mode;
    char session[TRANSFER_SESSION_CAPACITY];
    unsigned window_size;
    uint32_t timeout_ms;
    double loss_probability;
    double corruption_probability;
    double duplication_probability;
    uint16_t port;
    const char *relay_host;
    const char *file_path;
} transfer_configuration;
typedef enum
{
    TRANSFER_PARSE_OK,
    TRANSFER_PARSE_USAGE,
    TRANSFER_PARSE_ERROR
} transfer_parse_result;
/* #endregion Types */

/* #region Functions */
/** Parameters: argc/argv are writable process-style arguments; configuration
 *  points to caller-owned storage. Returns: OK for valid arguments, USAGE for
 *  argc == 1, ERROR otherwise (including NULL). Notes: getopt may permute argv;
 *  host/path borrow argv strings, which must stay alive during the transfer.
 *  Configuration is unchanged unless parsing succeeds. No owned memory is
 *  returned; diagnostics/usage are the caller's responsibility. */
transfer_parse_result parse_transfer_configuration(
    int argc,
    char *argv[],
    transfer_configuration *configuration);

/** Parameters: none. Returns: nothing. Notes: writes usage to stdout;
 *  owns no resources and needs no cleanup. */
void print_transfer_usage(void);

/** Parameters: configuration is a borrowed valid parsed SEND configuration.
 *  Returns: 0 on acknowledged FIN, 2 on invalid configuration, file, allocation,
 *  registration, network or protocol failure. Notes: errors go to stderr;
 *  loads at most 16 MiB; uses one socket throughout. Files, sockets, buffers
 *  and protocol state are released before returning. */
int run_sender_transfer(const transfer_configuration *configuration);

/** Parameters: configuration is a borrowed valid parsed RECEIVE configuration.
 *  Returns: 0 after FIN/linger, 2 on invalid configuration, file, registration,
 *  network, allocation or idle failure. Notes: truncates output after successful
 *  registration; interrupted runs may leave partial files. Uses one socket
 *  throughout and releases every resource before returning. */
int run_receiver_transfer(const transfer_configuration *configuration);
/* #endregion Functions */
#endif

```

## Tests Files
### lab-test.c

```c

#include "harness/unity.h"
#include "../src/lab.h"
#include <stdlib.h>
#include <string.h>

/* #region Constants */
#define SIMULATION_CAPACITY 4096U
/* #endregion Constants */
/* #region Types */
typedef struct
{
    encoded_protocol_packet packets[SIMULATION_CAPACITY];
    size_t count;
    uint32_t random_state;
    unsigned dropped;
    unsigned corrupted;
    unsigned duplicated;
} simulated_channel;
/* #endregion Types */

/* #region Helpers */
/* xorshift32 gives reproducible damage independently of the platform's rand(). */
static uint32_t next_simulation_random(simulated_channel *channel)
{
    uint32_t value = channel->random_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    channel->random_state = value;
    return value;
}

/* Apply relay-style conditional 20% loss, corruption and duplication to encoded bytes. */
static void enqueue_simulated_packet(
    simulated_channel *channel,
    const encoded_protocol_packet *packet)
{
    if (next_simulation_random(channel) % 100 < 20)
    {
        channel->dropped++;
        return;
    }
    TEST_ASSERT_LESS_THAN(SIMULATION_CAPACITY - 1, channel->count);
    encoded_protocol_packet damaged = *packet;
    if (next_simulation_random(channel) % 100 < 20)
    {
        size_t byte_index = next_simulation_random(channel) % damaged.size;
        damaged.bytes[byte_index] ^= (uint8_t)(1U << (next_simulation_random(channel) % 8));
        channel->corrupted++;
    }
    else if (next_simulation_random(channel) % 100 < 20)
    {
        channel->packets[channel->count++] = damaged;
        channel->duplicated++;
    }
    channel->packets[channel->count++] = damaged;
}

/* Decode an action packet, asserting that state-machine output is well formed. */
static protocol_packet decode_action(const protocol_actions *actions, size_t index)
{
    protocol_packet packet;
    TEST_ASSERT_LESS_THAN(actions->packet_count, index);
    TEST_ASSERT_TRUE(decode_and_validate_protocol_packet(
        actions->packets[index].bytes, actions->packets[index].size, &packet));
    return packet;
}

static void assert_acknowledgment(const protocol_actions *actions, uint32_t sequence_number)
{
    TEST_ASSERT_EQUAL(1, actions->packet_count);
    protocol_packet acknowledgment = decode_action(actions, 0);
    TEST_ASSERT_EQUAL(PROTOCOL_ACK, acknowledgment.type);
    TEST_ASSERT_EQUAL_UINT32(sequence_number, acknowledgment.sequence_number);
}

/* Repair a checksum so malformed-field tests reach field validation independently. */
static void repair_checksum(encoded_protocol_packet *packet)
{
    packet->bytes[2] = 0;
    packet->bytes[3] = 0;
    uint16_t checksum = calculate_internet_checksum(packet->bytes, packet->size);
    packet->bytes[2] = (uint8_t)(checksum >> 8);
    packet->bytes[3] = (uint8_t)checksum;
}
/* #endregion Helpers */

/* #region Functions */
void setUp(void)
{
}

void tearDown(void)
{
}

static void test_checksum_examples(void)
{
    const uint8_t rfc_example[] = {0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7};
    const uint8_t odd_example[] = {0x48, 0x69, 0x21};
    const uint8_t carry_example[] = {0xff, 0xff, 0xff, 0xff, 0x00, 0x01};
    TEST_ASSERT_EQUAL_HEX16(0x220d, calculate_internet_checksum(rfc_example, sizeof(rfc_example)));
    TEST_ASSERT_EQUAL_HEX16(0x9696, calculate_internet_checksum(odd_example, sizeof(odd_example)));
    TEST_ASSERT_EQUAL_HEX16(0xfffe, calculate_internet_checksum(carry_example, sizeof(carry_example)));
    TEST_ASSERT_EQUAL_HEX16(0xffff, calculate_internet_checksum(NULL, 0));
    TEST_ASSERT_EQUAL_HEX16(0, calculate_internet_checksum(NULL, 1));
}

static void test_packet_worked_examples_and_round_trip(void)
{
    protocol_packet packet = {.type = PROTOCOL_DATA, .sequence_number = 2,
                              .payload_length = 3, .payload = {'H', 'i', '!'}};
    const uint8_t example[] = {0, 0, 0x96, 0x91, 0, 0, 0, 2, 0, 3, 'H', 'i', '!'};
    encoded_protocol_packet encoded;
    protocol_packet decoded;
    TEST_ASSERT_TRUE(encode_protocol_packet(&packet, &encoded));
    TEST_ASSERT_EQUAL(sizeof(example), encoded.size);
    TEST_ASSERT_EQUAL_MEMORY(example, encoded.bytes, sizeof(example));
    TEST_ASSERT_EQUAL_HEX16(0, calculate_internet_checksum(encoded.bytes, encoded.size));
    TEST_ASSERT_TRUE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size, &decoded));
    TEST_ASSERT_EQUAL_UINT32(2, decoded.sequence_number);
    TEST_ASSERT_EQUAL_UINT16(3, decoded.payload_length);
    TEST_ASSERT_EQUAL_MEMORY(packet.payload, decoded.payload, 3);
    packet.type = PROTOCOL_ACK;
    packet.sequence_number = 3;
    packet.payload_length = 0;
    const uint8_t ack_example[] = {1, 0, 0xfe, 0xfc, 0, 0, 0, 3, 0, 0};
    TEST_ASSERT_TRUE(encode_protocol_packet(&packet, &encoded));
    TEST_ASSERT_EQUAL_MEMORY(ack_example, encoded.bytes, sizeof(ack_example));
    packet.type = PROTOCOL_FIN;
    packet.sequence_number = 0x12345678;
    TEST_ASSERT_TRUE(encode_protocol_packet(&packet, &encoded));
    TEST_ASSERT_TRUE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size, &decoded));
    TEST_ASSERT_EQUAL(PROTOCOL_FIN, decoded.type);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, decoded.sequence_number);
    packet.type = PROTOCOL_DATA;
    packet.payload_length = PROTOCOL_PAYLOAD_SIZE;
    memset(packet.payload, 0xff, sizeof(packet.payload));
    TEST_ASSERT_TRUE(encode_protocol_packet(&packet, &encoded));
    TEST_ASSERT_EQUAL(PROTOCOL_PACKET_SIZE, encoded.size);
    TEST_ASSERT_TRUE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size, &decoded));
    TEST_ASSERT_EQUAL_MEMORY(packet.payload, decoded.payload, PROTOCOL_PAYLOAD_SIZE);
}

static void test_packet_invalid_arguments_and_fields(void)
{
    protocol_packet packet = {.type = PROTOCOL_DATA, .payload_length = 1, .payload = {0x37}};
    protocol_packet decoded;
    memset(&decoded, 0x5a, sizeof(decoded));
    protocol_packet untouched = decoded;
    encoded_protocol_packet encoded = {0};
    TEST_ASSERT_FALSE(encode_protocol_packet(NULL, &encoded));
    TEST_ASSERT_FALSE(encode_protocol_packet(&packet, NULL));
    packet.type = (protocol_packet_type)-1;
    TEST_ASSERT_FALSE(encode_protocol_packet(&packet, &encoded));
    packet.type = (protocol_packet_type)3;
    TEST_ASSERT_FALSE(encode_protocol_packet(&packet, &encoded));
    packet.type = PROTOCOL_DATA;
    packet.payload_length = 1025;
    TEST_ASSERT_FALSE(encode_protocol_packet(&packet, &encoded));
    packet.payload_length = 1;
    packet.type = PROTOCOL_ACK;
    TEST_ASSERT_FALSE(encode_protocol_packet(&packet, &encoded));
    packet.type = PROTOCOL_FIN;
    TEST_ASSERT_FALSE(encode_protocol_packet(&packet, &encoded));
    packet.type = PROTOCOL_DATA;
    TEST_ASSERT_TRUE(encode_protocol_packet(&packet, &encoded));
    TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(NULL, encoded.size, &decoded));
    TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size, NULL));
    TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(encoded.bytes, 9, &decoded));
    TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(encoded.bytes, PROTOCOL_PACKET_SIZE + 1, &decoded));
    TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size - 1, &decoded));
    encoded_protocol_packet valid = encoded;
    for (size_t bit = 0; bit < encoded.size * 8; bit++)
    {
        encoded = valid;
        encoded.bytes[bit / 8] ^= (uint8_t)(1U << (bit % 8));
        TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size, &decoded));
    }
    for (unsigned field = 0; field < 5; field++)
    {
        encoded = valid;
        if (field == 0)
        {
            encoded.bytes[0] = 3;
        }
        else if (field == 1)
        {
            encoded.bytes[1] = 1;
        }
        else if (field == 2)
        {
            encoded.bytes[8] = 4;
            encoded.bytes[9] = 1;
        }
        else
        {
            encoded.bytes[0] = field == 3 ? PROTOCOL_ACK : PROTOCOL_FIN;
        }
        repair_checksum(&encoded);
        TEST_ASSERT_FALSE(decode_and_validate_protocol_packet(encoded.bytes, encoded.size, &decoded));
    }
    TEST_ASSERT_EQUAL_MEMORY(&untouched, &decoded, sizeof(decoded));
}

static void test_receiver_order_duplicates_gaps_and_finish(void)
{
    receiver_protocol_state *receiver = create_receiver_protocol_state(100);
    TEST_ASSERT_NOT_NULL(receiver);
    protocol_actions actions;
    protocol_packet packet = {.type = PROTOCOL_DATA, .sequence_number = 1,
                              .payload_length = 3, .payload = {'a', 'b', 'c'}};
    process_receiver_protocol_packet(receiver, &packet, 200, &actions);
    assert_acknowledgment(&actions, 0);
    TEST_ASSERT_EQUAL(0, actions.delivery_size);
    TEST_ASSERT_EQUAL_UINT64(30200, actions.deadline_ms);
    packet.sequence_number = 0;
    process_receiver_protocol_packet(receiver, &packet, 300, &actions);
    assert_acknowledgment(&actions, 1);
    TEST_ASSERT_EQUAL_MEMORY(packet.payload, actions.delivery, 3);
    TEST_ASSERT_EQUAL(3, actions.delivery_size);
    process_receiver_protocol_packet(receiver, &packet, 400, &actions);
    assert_acknowledgment(&actions, 1);
    TEST_ASSERT_EQUAL(0, actions.delivery_size);
    packet.type = PROTOCOL_FIN;
    packet.payload_length = 0;
    packet.sequence_number = 2;
    process_receiver_protocol_packet(receiver, &packet, 500, &actions);
    assert_acknowledgment(&actions, 1);
    TEST_ASSERT_FALSE(actions.close_output);
    packet.sequence_number = 1;
    process_receiver_protocol_packet(receiver, &packet, 600, &actions);
    assert_acknowledgment(&actions, 2);
    TEST_ASSERT_TRUE(actions.close_output);
    TEST_ASSERT_EQUAL_UINT64(2600, actions.deadline_ms);
    process_receiver_protocol_packet(receiver, &packet, 700, &actions);
    assert_acknowledgment(&actions, 2);
    TEST_ASSERT_FALSE(actions.close_output);
    TEST_ASSERT_EQUAL_UINT64(2600, actions.deadline_ms);
    packet.type = PROTOCOL_DATA;
    packet.payload_length = 3;
    packet.sequence_number = 2;
    process_receiver_protocol_packet(receiver, &packet, 800, &actions);
    assert_acknowledgment(&actions, 2);
    TEST_ASSERT_EQUAL(0, actions.delivery_size);
    packet.type = PROTOCOL_ACK;
    process_receiver_protocol_packet(receiver, &packet, 900, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    process_receiver_protocol_timeout(receiver, 2599, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_ACTIVE, actions.status);
    process_receiver_protocol_timeout(receiver, 2600, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    TEST_ASSERT_EQUAL_UINT64(PROTOCOL_NO_DEADLINE, actions.deadline_ms);
    process_receiver_protocol_packet(receiver, &packet, 2601, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    destroy_receiver_protocol_state(receiver);
}

static void test_receiver_idle_and_packet_deadline(void)
{
    protocol_actions actions;
    protocol_packet acknowledgment = {.type = PROTOCOL_ACK};
    receiver_protocol_state *receiver = create_receiver_protocol_state(0);
    process_receiver_protocol_timeout(receiver, 29999, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_ACTIVE, actions.status);
    process_receiver_protocol_packet(receiver, &acknowledgment, 29999, &actions);
    TEST_ASSERT_EQUAL_UINT64(59999, actions.deadline_ms);
    process_receiver_protocol_timeout(receiver, 59999, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    process_receiver_protocol_packet(receiver, &acknowledgment, 60000, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    destroy_receiver_protocol_state(receiver);
    receiver = create_receiver_protocol_state(0);
    process_receiver_protocol_packet(receiver, &acknowledgment, 30000, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    destroy_receiver_protocol_state(receiver);
    receiver = create_receiver_protocol_state(0);
    protocol_packet finish = {.type = PROTOCOL_FIN};
    process_receiver_protocol_packet(receiver, &finish, 1, &actions);
    process_receiver_protocol_packet(receiver, &finish, 2001, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    destroy_receiver_protocol_state(receiver);
}

static void test_receiver_input_limit_and_saturated_clock(void)
{
    receiver_protocol_state *receiver = create_receiver_protocol_state(UINT64_MAX - 10);
    protocol_actions actions;
    process_receiver_protocol_timeout(receiver, UINT64_MAX - 2, &actions);
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX - 1, actions.deadline_ms);
    destroy_receiver_protocol_state(receiver);
    receiver = create_receiver_protocol_state(0);
    protocol_packet packet = {.type = PROTOCOL_DATA, .payload_length = PROTOCOL_PAYLOAD_SIZE};
    for (uint32_t index = 0; index < PROTOCOL_MAX_INPUT_SIZE / PROTOCOL_PAYLOAD_SIZE; index++)
    {
        packet.sequence_number = index;
        process_receiver_protocol_packet(receiver, &packet, index, &actions);
        TEST_ASSERT_EQUAL(PROTOCOL_ACTIVE, actions.status);
    }
    packet.sequence_number++;
    process_receiver_protocol_packet(receiver, &packet, 20000, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    destroy_receiver_protocol_state(receiver);
    /* Zero-length DATA is valid on the wire, but may not create unbounded sequences. */
    receiver = create_receiver_protocol_state(0);
    packet.payload_length = 0;
    for (uint32_t index = 0; index <= PROTOCOL_MAX_INPUT_SIZE / PROTOCOL_PAYLOAD_SIZE; index++)
    {
        packet.sequence_number = index;
        process_receiver_protocol_packet(receiver, &packet, index, &actions);
    }
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    destroy_receiver_protocol_state(receiver);
}

static void test_protocol_null_arguments(void)
{
    protocol_actions actions;
    protocol_packet packet = {.type = PROTOCOL_ACK};
    receiver_protocol_state *receiver = create_receiver_protocol_state(0);
    sender_protocol_state *sender = create_sender_protocol_state(NULL, 0, 1, 250);
    process_receiver_protocol_packet(receiver, &packet, 0, NULL);
    process_receiver_protocol_timeout(receiver, 0, NULL);
    start_sender_protocol_transfer(sender, 0, NULL);
    process_sender_protocol_acknowledgment(sender, &packet, 0, NULL);
    process_sender_protocol_timeout(sender, 0, NULL);
    process_receiver_protocol_packet(NULL, &packet, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    process_receiver_protocol_packet(receiver, NULL, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    process_receiver_protocol_timeout(NULL, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    start_sender_protocol_transfer(NULL, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    process_sender_protocol_acknowledgment(NULL, &packet, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    process_sender_protocol_acknowledgment(sender, NULL, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    process_sender_protocol_timeout(NULL, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    destroy_sender_protocol_state(sender);
    destroy_receiver_protocol_state(receiver);
    destroy_sender_protocol_state(NULL);
    destroy_receiver_protocol_state(NULL);
}

static void test_sender_window_acknowledgments_and_fin(void)
{
    uint8_t input[6 * PROTOCOL_PAYLOAD_SIZE];
    memset(input, 0x71, sizeof(input));
    sender_protocol_state *sender = create_sender_protocol_state(input, sizeof(input), 4, 250);
    TEST_ASSERT_NOT_NULL(sender);
    memset(input, 0x22, sizeof(input));
    protocol_actions actions;
    protocol_packet acknowledgment = {.type = PROTOCOL_ACK, .sequence_number = 3};
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 0, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(PROTOCOL_NO_DEADLINE, actions.deadline_ms);
    process_sender_protocol_timeout(sender, 0, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    start_sender_protocol_transfer(sender, 100, &actions);
    TEST_ASSERT_EQUAL(4, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(350, actions.deadline_ms);
    for (size_t index = 0; index < 4; index++)
    {
        protocol_packet packet = decode_action(&actions, index);
        TEST_ASSERT_EQUAL_UINT32(index, packet.sequence_number);
        TEST_ASSERT_EQUAL(1024, packet.payload_length);
        TEST_ASSERT_EQUAL_HEX8(0x71, packet.payload[0]);
    }
    encoded_protocol_packet retained[4];
    memcpy(retained, actions.packets, sizeof(retained));
    start_sender_protocol_transfer(sender, 101, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(350, actions.deadline_ms);
    acknowledgment.sequence_number = 5;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 150, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(350, actions.deadline_ms);
    acknowledgment.sequence_number = 0;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 150, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    process_sender_protocol_timeout(sender, 349, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    process_sender_protocol_timeout(sender, 350, &actions);
    TEST_ASSERT_EQUAL(4, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(600, actions.deadline_ms);
    for (size_t index = 0; index < 4; index++)
    {
        TEST_ASSERT_EQUAL(retained[index].size, actions.packets[index].size);
        TEST_ASSERT_EQUAL_MEMORY(retained[index].bytes, actions.packets[index].bytes, retained[index].size);
    }
    acknowledgment.sequence_number = 3;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 400, &actions);
    TEST_ASSERT_EQUAL(2, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT32(4, decode_action(&actions, 0).sequence_number);
    TEST_ASSERT_EQUAL_UINT32(5, decode_action(&actions, 1).sequence_number);
    TEST_ASSERT_EQUAL_UINT64(650, actions.deadline_ms);
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 401, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(650, actions.deadline_ms);
    acknowledgment.type = PROTOCOL_DATA;
    acknowledgment.sequence_number = 4;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 402, &actions);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    acknowledgment.type = PROTOCOL_ACK;
    acknowledgment.sequence_number = 6;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 450, &actions);
    TEST_ASSERT_EQUAL(1, actions.packet_count);
    protocol_packet finish = decode_action(&actions, 0);
    TEST_ASSERT_EQUAL(PROTOCOL_FIN, finish.type);
    TEST_ASSERT_EQUAL_UINT32(6, finish.sequence_number);
    TEST_ASSERT_EQUAL(0, finish.payload_length);
    process_sender_protocol_timeout(sender, 700, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FIN, decode_action(&actions, 0).type);
    acknowledgment.sequence_number = 7;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 701, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    TEST_ASSERT_EQUAL_UINT64(PROTOCOL_NO_DEADLINE, actions.deadline_ms);
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 702, &actions);
    process_sender_protocol_timeout(sender, 1000, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    start_sender_protocol_transfer(sender, 1001, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    destroy_sender_protocol_state(sender);
}

static void test_sender_empty_partial_and_window_limits(void)
{
    TEST_ASSERT_NULL(create_sender_protocol_state(NULL, 1, 1, 250));
    uint8_t byte = 0x11;
    TEST_ASSERT_NULL(create_sender_protocol_state(&byte, PROTOCOL_MAX_INPUT_SIZE + 1, 1, 250));
    TEST_ASSERT_NULL(create_sender_protocol_state(&byte, 1, 0, 250));
    TEST_ASSERT_NULL(create_sender_protocol_state(&byte, 1, 65, 250));
    TEST_ASSERT_NULL(create_sender_protocol_state(&byte, 1, 1, 0));
    protocol_actions actions;
    sender_protocol_state *sender = create_sender_protocol_state(NULL, 0, 64, 250);
    start_sender_protocol_transfer(sender, 0, &actions);
    TEST_ASSERT_EQUAL(1, actions.packet_count);
    TEST_ASSERT_EQUAL(PROTOCOL_FIN, decode_action(&actions, 0).type);
    TEST_ASSERT_EQUAL_UINT32(0, decode_action(&actions, 0).sequence_number);
    protocol_packet acknowledgment = {.type = PROTOCOL_ACK, .sequence_number = 1};
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 1, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, actions.status);
    destroy_sender_protocol_state(sender);
    uint8_t input[2500];
    memset(input, 0x19, sizeof(input));
    sender = create_sender_protocol_state(input, sizeof(input), 1, 250);
    start_sender_protocol_transfer(sender, 0, &actions);
    TEST_ASSERT_EQUAL(1, actions.packet_count);
    TEST_ASSERT_EQUAL(1024, decode_action(&actions, 0).payload_length);
    for (uint32_t sequence = 1; sequence <= 2; sequence++)
    {
        acknowledgment.sequence_number = sequence;
        process_sender_protocol_acknowledgment(sender, &acknowledgment, sequence, &actions);
        TEST_ASSERT_EQUAL(1, actions.packet_count);
    }
    TEST_ASSERT_EQUAL(452, decode_action(&actions, 0).payload_length);
    acknowledgment.sequence_number = 3;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 3, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FIN, decode_action(&actions, 0).type);
    destroy_sender_protocol_state(sender);
    uint8_t full_window[64 * PROTOCOL_PAYLOAD_SIZE];
    memset(full_window, 0x36, sizeof(full_window));
    sender = create_sender_protocol_state(full_window, sizeof(full_window), 64, 250);
    start_sender_protocol_transfer(sender, 0, &actions);
    TEST_ASSERT_EQUAL(64, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT32(63, decode_action(&actions, 63).sequence_number);
    acknowledgment.sequence_number = 64;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 1, &actions);
    TEST_ASSERT_EQUAL(1, actions.packet_count);
    TEST_ASSERT_EQUAL(PROTOCOL_FIN, decode_action(&actions, 0).type);
    TEST_ASSERT_EQUAL_UINT32(64, decode_action(&actions, 0).sequence_number);
    destroy_sender_protocol_state(sender);
}

static void test_sender_failure_and_progress_reset(void)
{
    uint8_t input[2048] = {0};
    sender_protocol_state *sender = create_sender_protocol_state(input, sizeof(input), 1, 10);
    protocol_actions actions;
    start_sender_protocol_transfer(sender, 0, &actions);
    for (uint64_t timeout = 1; timeout <= 9; timeout++)
    {
        process_sender_protocol_timeout(sender, timeout * 10, &actions);
        TEST_ASSERT_EQUAL(PROTOCOL_ACTIVE, actions.status);
        TEST_ASSERT_EQUAL(1, actions.packet_count);
    }
    protocol_packet acknowledgment = {.type = PROTOCOL_ACK, .sequence_number = 1};
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 91, &actions);
    TEST_ASSERT_EQUAL_UINT64(101, actions.deadline_ms);
    for (uint64_t timeout = 1; timeout <= 9; timeout++)
    {
        process_sender_protocol_timeout(sender, 91 + timeout * 10, &actions);
        TEST_ASSERT_EQUAL(PROTOCOL_ACTIVE, actions.status);
        process_sender_protocol_acknowledgment(sender, &acknowledgment, 91 + timeout * 10, &actions);
    }
    process_sender_protocol_timeout(sender, 191, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    TEST_ASSERT_EQUAL(0, actions.packet_count);
    TEST_ASSERT_EQUAL_UINT64(PROTOCOL_NO_DEADLINE, actions.deadline_ms);
    acknowledgment.sequence_number = 2;
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 200, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_FAILED, actions.status);
    destroy_sender_protocol_state(sender);
}

static void test_lost_finish_acknowledgment_recovery(void)
{
    sender_protocol_state *sender = create_sender_protocol_state(NULL, 0, 1, 250);
    receiver_protocol_state *receiver = create_receiver_protocol_state(0);
    protocol_actions sender_actions;
    protocol_actions receiver_actions;
    start_sender_protocol_transfer(sender, 0, &sender_actions);
    protocol_packet finish = decode_action(&sender_actions, 0);
    process_receiver_protocol_packet(receiver, &finish, 50, &receiver_actions);
    TEST_ASSERT_TRUE(receiver_actions.close_output);
    /* Drop this ACK, then retransmit FIN through encoding and decoding. */
    process_sender_protocol_timeout(sender, 250, &sender_actions);
    finish = decode_action(&sender_actions, 0);
    process_receiver_protocol_packet(receiver, &finish, 300, &receiver_actions);
    TEST_ASSERT_FALSE(receiver_actions.close_output);
    TEST_ASSERT_EQUAL_UINT64(2050, receiver_actions.deadline_ms);
    protocol_packet acknowledgment = decode_action(&receiver_actions, 0);
    process_sender_protocol_acknowledgment(sender, &acknowledgment, 350, &sender_actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, sender_actions.status);
    process_receiver_protocol_timeout(receiver, 2050, &receiver_actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, receiver_actions.status);
    destroy_sender_protocol_state(sender);
    destroy_receiver_protocol_state(receiver);
}

/* Drive full encoded transfers with fake time, no sockets, files or clock reads. */
static void run_simulated_transfer(uint32_t seed, size_t input_size, size_t window_size)
{
    uint8_t *input = malloc(input_size + 1);
    uint8_t *output = malloc(input_size + 1);
    simulated_channel *to_receiver = calloc(1, sizeof(*to_receiver));
    simulated_channel *to_sender = calloc(1, sizeof(*to_sender));
    TEST_ASSERT_NOT_NULL(input);
    TEST_ASSERT_NOT_NULL(output);
    TEST_ASSERT_NOT_NULL(to_receiver);
    TEST_ASSERT_NOT_NULL(to_sender);
    for (size_t index = 0; index < input_size; index++)
    {
        input[index] = (uint8_t)(index * 37 + index / 1024);
    }
    to_receiver->random_state = seed;
    to_sender->random_state = seed ^ 0x9e3779b9U;
    sender_protocol_state *sender = create_sender_protocol_state(input, input_size, window_size, 250);
    receiver_protocol_state *receiver = create_receiver_protocol_state(0);
    TEST_ASSERT_NOT_NULL(sender);
    TEST_ASSERT_NOT_NULL(receiver);
    protocol_actions sender_actions;
    protocol_actions receiver_actions;
    size_t output_size = 0;
    unsigned closures = 0;
    uint64_t now_ms = 0;
    start_sender_protocol_transfer(sender, now_ms, &sender_actions);
    for (size_t index = 0; index < sender_actions.packet_count; index++)
    {
        enqueue_simulated_packet(to_receiver, &sender_actions.packets[index]);
    }
    for (unsigned round = 0; round < 100000 && sender_actions.status == PROTOCOL_ACTIVE; round++)
    {
        now_ms += 10;
        for (size_t index = 0; index < to_receiver->count; index++)
        {
            protocol_packet packet;
            encoded_protocol_packet *encoded = &to_receiver->packets[index];
            if (!decode_and_validate_protocol_packet(encoded->bytes, encoded->size, &packet))
            {
                continue;
            }
            process_receiver_protocol_packet(receiver, &packet, now_ms, &receiver_actions);
            TEST_ASSERT_EQUAL(PROTOCOL_ACTIVE, receiver_actions.status);
            TEST_ASSERT_LESS_OR_EQUAL(input_size, output_size + receiver_actions.delivery_size);
            memcpy(output + output_size, receiver_actions.delivery, receiver_actions.delivery_size);
            output_size += receiver_actions.delivery_size;
            closures += receiver_actions.close_output ? 1U : 0U;
            for (size_t ack_index = 0; ack_index < receiver_actions.packet_count; ack_index++)
            {
                enqueue_simulated_packet(to_sender, &receiver_actions.packets[ack_index]);
            }
        }
        to_receiver->count = 0;
        for (size_t index = 0; index < to_sender->count; index++)
        {
            protocol_packet packet;
            encoded_protocol_packet *encoded = &to_sender->packets[index];
            if (!decode_and_validate_protocol_packet(encoded->bytes, encoded->size, &packet))
            {
                continue;
            }
            process_sender_protocol_acknowledgment(sender, &packet, now_ms, &sender_actions);
            for (size_t send_index = 0; send_index < sender_actions.packet_count; send_index++)
            {
                enqueue_simulated_packet(to_receiver, &sender_actions.packets[send_index]);
            }
        }
        to_sender->count = 0;
        if (sender_actions.status == PROTOCOL_ACTIVE && now_ms >= sender_actions.deadline_ms)
        {
            process_sender_protocol_timeout(sender, now_ms, &sender_actions);
            for (size_t index = 0; index < sender_actions.packet_count; index++)
            {
                enqueue_simulated_packet(to_receiver, &sender_actions.packets[index]);
            }
        }
    }
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, sender_actions.status);
    TEST_ASSERT_EQUAL(input_size, output_size);
    if (input_size != 0)
    {
        TEST_ASSERT_EQUAL_MEMORY(input, output, input_size);
    }
    TEST_ASSERT_EQUAL(1, closures);
    process_receiver_protocol_timeout(receiver, now_ms + 2000, &receiver_actions);
    TEST_ASSERT_EQUAL(PROTOCOL_COMPLETE, receiver_actions.status);
    if (input_size > 4096)
    {
        TEST_ASSERT_GREATER_THAN(0, to_receiver->dropped);
        TEST_ASSERT_GREATER_THAN(0, to_receiver->corrupted);
        TEST_ASSERT_GREATER_THAN(0, to_receiver->duplicated);
        TEST_ASSERT_GREATER_THAN(0, to_sender->dropped);
        TEST_ASSERT_GREATER_THAN(0, to_sender->corrupted);
        TEST_ASSERT_GREATER_THAN(0, to_sender->duplicated);
    }
    destroy_sender_protocol_state(sender);
    destroy_receiver_protocol_state(receiver);
    free(to_receiver);
    free(to_sender);
    free(input);
    free(output);
}

static void test_seeded_damaged_transfers(void)
{
    run_simulated_transfer(7, 65573, 16);
    run_simulated_transfer(42, 65536, 1);
    run_simulated_transfer(12345, 131101, 64);
    run_simulated_transfer(99, 0, 8);
    uint8_t *maximum_input = calloc(1, PROTOCOL_MAX_INPUT_SIZE);
    TEST_ASSERT_NOT_NULL(maximum_input);
    sender_protocol_state *maximum_sender = create_sender_protocol_state(
        maximum_input, PROTOCOL_MAX_INPUT_SIZE, PROTOCOL_MAX_WINDOW, 250);
    TEST_ASSERT_NOT_NULL(maximum_sender);
    protocol_actions actions;
    start_sender_protocol_transfer(maximum_sender, 0, &actions);
    TEST_ASSERT_EQUAL(PROTOCOL_MAX_WINDOW, actions.packet_count);
    destroy_sender_protocol_state(maximum_sender);
    free(maximum_input);
}

/* Exported from the second test source; all tests still use the existing Unity runner. */
void run_transfer_io_tests(void);

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_checksum_examples);
    RUN_TEST(test_packet_worked_examples_and_round_trip);
    RUN_TEST(test_packet_invalid_arguments_and_fields);
    RUN_TEST(test_receiver_order_duplicates_gaps_and_finish);
    RUN_TEST(test_receiver_idle_and_packet_deadline);
    RUN_TEST(test_receiver_input_limit_and_saturated_clock);
    RUN_TEST(test_protocol_null_arguments);
    RUN_TEST(test_sender_window_acknowledgments_and_fin);
    RUN_TEST(test_sender_empty_partial_and_window_limits);
    RUN_TEST(test_sender_failure_and_progress_reset);
    RUN_TEST(test_lost_finish_acknowledgment_recovery);
    RUN_TEST(test_seeded_damaged_transfers);
    run_transfer_io_tests();
    return UNITY_END();
}
/* #endregion Functions */

```

### transfer-io-test.c

```c

#define _POSIX_C_SOURCE 200809L
#include "harness/unity.h"
#include "../src/lab.h"
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

/* #region Constants */
/* #endregion Constants */
/* #region Types */
typedef enum
{
    STUB_SENDER,
    STUB_RECEIVER,
    STUB_RETRY,
    STUB_SILENT,
    STUB_REFUSE,
    STUB_NO_ACK,
    STUB_CLOSE,
    STUB_RECEIVER_IDLE
} relay_stub_mode;

typedef struct
{
    pid_t process_id;
    uint16_t port;
} relay_stub;
/* #endregion Types */

/* #region Helpers */
/* Parse a private mutable copy, because getopt is allowed to reorder argv pointers. */
static transfer_parse_result parse_test_arguments(
    size_t count,
    const char *arguments[],
    transfer_configuration *configuration)
{
    char *mutable_arguments[32];
    TEST_ASSERT_LESS_THAN(32, count);
    for (size_t index = 0; index < count; index++)
    {
        mutable_arguments[index] = (char *)arguments[index];
    }
    mutable_arguments[count] = NULL;
    return parse_transfer_configuration((int)count, mutable_arguments, configuration);
}

/* Child-process assertion: exit without using the parent's Unity or coverage state. */
static void require_stub_condition(bool condition)
{
    if (!condition)
    {
        _exit(3);
    }
}

/* Send one encoded datagram and require its cumulative ACK before sending another. */
static void exchange_receiver_packet(
    int socket_descriptor,
    const struct sockaddr_in *client,
    const protocol_packet *packet,
    uint32_t expected_acknowledgment)
{
    encoded_protocol_packet encoded;
    require_stub_condition(encode_protocol_packet(packet, &encoded));
    require_stub_condition(sendto(socket_descriptor, encoded.bytes, encoded.size, 0,
                                 (const struct sockaddr *)client, sizeof(*client)) >= 0);
    uint8_t bytes[PROTOCOL_PACKET_SIZE + 1];
    ssize_t size = recv(socket_descriptor, bytes, sizeof(bytes), 0);
    protocol_packet acknowledgment;
    require_stub_condition(size >= 0 && decode_and_validate_protocol_packet(bytes, (size_t)size, &acknowledgment));
    require_stub_condition(acknowledgment.type == PROTOCOL_ACK &&
                           acknowledgment.sequence_number == expected_acknowledgment);
}

/* Controlled local UDP peer for testing public I/O entry points, with no Python relay dependency. */
static void serve_relay_stub(int socket_descriptor, relay_stub_mode mode)
{
    struct timeval receive_timeout = {.tv_sec = 8};
    require_stub_condition(setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO,
                                     &receive_timeout, sizeof(receive_timeout)) == 0);
    uint8_t bytes[2048];
    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);
    ssize_t size = recvfrom(socket_descriptor, bytes, sizeof(bytes), 0,
                            (struct sockaddr *)&client, &client_size);
    require_stub_condition(size > 0 && memcmp(bytes, "HELLO ", 6) == 0);
    if (mode == STUB_SILENT)
    {
        for (unsigned attempt = 1; attempt < 5; attempt++)
        {
            size = recvfrom(socket_descriptor, bytes, sizeof(bytes), 0,
                            (struct sockaddr *)&client, &client_size);
            require_stub_condition(size > 0);
        }
        /* Keep the UDP port bound until the fifth attempt's one-second wait ends. */
        sleep(2);
        _exit(0);
    }
    if (mode == STUB_REFUSE)
    {
        require_stub_condition(sendto(socket_descriptor, "ERR test refusal", 16, 0,
                                     (struct sockaddr *)&client, client_size) == 16);
        _exit(0);
    }
    if (mode == STUB_RETRY)
    {
        require_stub_condition(sendto(socket_descriptor, "unrelated", 9, 0,
                                     (struct sockaddr *)&client, client_size) == 9);
        struct sockaddr_in retry_client;
        socklen_t retry_size = sizeof(retry_client);
        size = recvfrom(socket_descriptor, bytes, sizeof(bytes), 0,
                        (struct sockaddr *)&retry_client, &retry_size);
        require_stub_condition(size > 0 && retry_client.sin_port == client.sin_port);
    }
    require_stub_condition(sendto(socket_descriptor, "OK", 2, 0,
                                 (struct sockaddr *)&client, client_size) == 2);
    if (mode == STUB_CLOSE)
    {
        close(socket_descriptor);
        _exit(0);
    }
    if (mode == STUB_RECEIVER_IDLE)
    {
        sleep(31);
        _exit(0);
    }
    if (mode == STUB_RECEIVER)
    {
        /* Invalid short and oversized datagrams must produce no ACK. */
        require_stub_condition(sendto(socket_descriptor, "OK", 2, 0,
                                     (struct sockaddr *)&client, client_size) == 2);
        memset(bytes, 0, sizeof(bytes));
        require_stub_condition(sendto(socket_descriptor, bytes, sizeof(bytes), 0,
                                     (struct sockaddr *)&client, client_size) == (ssize_t)sizeof(bytes));
        protocol_packet packet = {.type = PROTOCOL_DATA, .sequence_number = 1,
                                  .payload_length = 3, .payload = {0, 0xff, 0x37}};
        exchange_receiver_packet(socket_descriptor, &client, &packet, 0);
        packet.sequence_number = 0;
        exchange_receiver_packet(socket_descriptor, &client, &packet, 1);
        exchange_receiver_packet(socket_descriptor, &client, &packet, 1);
        packet.type = PROTOCOL_FIN;
        packet.payload_length = 0;
        packet.sequence_number = 1;
        exchange_receiver_packet(socket_descriptor, &client, &packet, 2);
        exchange_receiver_packet(socket_descriptor, &client, &packet, 2);
        _exit(0);
    }
    unsigned received_packets = 0;
    for (;;)
    {
        client_size = sizeof(client);
        size = recvfrom(socket_descriptor, bytes, sizeof(bytes), 0,
                        (struct sockaddr *)&client, &client_size);
        require_stub_condition(size > 0);
        protocol_packet packet;
        require_stub_condition(decode_and_validate_protocol_packet(bytes, (size_t)size, &packet));
        received_packets++;
        if (mode == STUB_NO_ACK)
        {
            if (received_packets == 10)
            {
                sleep(1);
                _exit(0);
            }
            continue;
        }
        protocol_packet acknowledgment = {.type = PROTOCOL_ACK,
            .sequence_number = packet.sequence_number + 1};
        encoded_protocol_packet encoded;
        require_stub_condition(encode_protocol_packet(&acknowledgment, &encoded));
        if (received_packets == 1)
        {
            encoded.bytes[2] ^= 1;
            require_stub_condition(sendto(socket_descriptor, encoded.bytes, encoded.size, 0,
                                         (struct sockaddr *)&client, client_size) >= 0);
            encoded.bytes[2] ^= 1;
        }
        require_stub_condition(sendto(socket_descriptor, encoded.bytes, encoded.size, 0,
                                     (struct sockaddr *)&client, client_size) >= 0);
        if (packet.type == PROTOCOL_FIN)
        {
            _exit(0);
        }
    }
}

/* Own a forked stub and an OS-assigned local port; parent closes its socket copy. */
static relay_stub create_relay_stub(relay_stub_mode mode)
{
    int socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, socket_descriptor);
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    TEST_ASSERT_EQUAL(0, bind(socket_descriptor, (struct sockaddr *)&address, sizeof(address)));
    socklen_t address_size = sizeof(address);
    TEST_ASSERT_EQUAL(0, getsockname(socket_descriptor, (struct sockaddr *)&address, &address_size));
    pid_t process_id = fork();
    TEST_ASSERT_GREATER_OR_EQUAL(0, process_id);
    if (process_id == 0)
    {
        serve_relay_stub(socket_descriptor, mode);
        _exit(0);
    }
    close(socket_descriptor);
    relay_stub stub = {.process_id = process_id, .port = ntohs(address.sin_port)};
    return stub;
}

/* Reap and verify the owned child; no child process is left after a successful test. */
static void destroy_relay_stub(relay_stub *stub)
{
    int status;
    TEST_ASSERT_EQUAL(stub->process_id, waitpid(stub->process_id, &status, 0));
    TEST_ASSERT_TRUE(WIFEXITED(status));
    TEST_ASSERT_EQUAL(0, WEXITSTATUS(status));
    stub->process_id = -1;
}

/* Return a fully initialized borrowed configuration for a temporary local transfer. */
static transfer_configuration create_test_configuration(
    transfer_mode mode,
    uint16_t port,
    const char *file_path)
{
    transfer_configuration configuration = {
        .mode = mode, .session = "unit-test", .window_size = 8,
        .timeout_ms = 10, .port = port,
        .relay_host = "127.0.0.1", .file_path = file_path
    };
    return configuration;
}

/* Create a unique owned temporary file. The caller closes the stream and unlinks path. */
static FILE *create_test_file(char path[32])
{
    strcpy(path, "/tmp/cs425-unity-XXXXXX");
    int descriptor = mkstemp(path);
    TEST_ASSERT_GREATER_OR_EQUAL(0, descriptor);
    FILE *file = fdopen(descriptor, "w+b");
    TEST_ASSERT_NOT_NULL(file);
    return file;
}

/* A harmless signal makes poll prove that it recomputes its absolute deadline. */
static void handle_test_signal(int signal_number)
{
    (void)signal_number;
}
/* #endregion Helpers */

/* #region Functions */
static void test_cli_defaults_and_options(void)
{
    transfer_configuration configuration;
    const char *send_arguments[] = {"myapp", "send", "-s", "test-1", "localhost", "in.bin"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_OK, parse_test_arguments(6, send_arguments, &configuration));
    TEST_ASSERT_EQUAL(TRANSFER_SEND, configuration.mode);
    TEST_ASSERT_EQUAL_STRING("test-1", configuration.session);
    TEST_ASSERT_EQUAL(8, configuration.window_size);
    TEST_ASSERT_EQUAL(250, configuration.timeout_ms);
    TEST_ASSERT_EQUAL(4250, configuration.port);
    TEST_ASSERT_EQUAL_STRING("localhost", configuration.relay_host);
    TEST_ASSERT_EQUAL_STRING("in.bin", configuration.file_path);
    const char *options[] = {"myapp", "send", "-s", "abcdefghijklmnopqrstuvwxyz123456",
        "-w", "64", "-T", "4294967295", "-l", "0.5", "-c", ".2", "-d", "0", "-p", "65535", "127.0.0.1", "input"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_OK, parse_test_arguments(18, options, &configuration));
    TEST_ASSERT_EQUAL(64, configuration.window_size);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, configuration.timeout_ms);
    TEST_ASSERT_EQUAL(65535, configuration.port);
    TEST_ASSERT_TRUE(configuration.loss_probability == .5);
    TEST_ASSERT_TRUE(configuration.corruption_probability == .2);
    TEST_ASSERT_TRUE(configuration.duplication_probability == 0);
    const char *receive_arguments[] = {"myapp", "recv", "-s", "test-1", "-p", "20000", "localhost", "out.bin"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_OK, parse_test_arguments(8, receive_arguments, &configuration));
    TEST_ASSERT_EQUAL(TRANSFER_RECEIVE, configuration.mode);
    TEST_ASSERT_EQUAL(20000, configuration.port);
    print_transfer_usage();
}

static void test_cli_invalid_arguments(void)
{
    transfer_configuration configuration;
    char *usage[] = {"myapp", NULL};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_USAGE, parse_transfer_configuration(1, usage, &configuration));
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_transfer_configuration(0, usage, &configuration));
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_transfer_configuration(1, NULL, &configuration));
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_transfer_configuration(1, usage, NULL));
    const char *bad_mode[] = {"myapp", "other"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(2, bad_mode, &configuration));
    const char *missing_session[] = {"myapp", "send", "host", "file"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(4, missing_session, &configuration));
    const char *missing_file[] = {"myapp", "send", "-s", "ok", "host"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(5, missing_file, &configuration));
    const char *extra_file[] = {"myapp", "send", "-s", "ok", "host", "file", "extra"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(7, extra_file, &configuration));
    const char *missing_option[] = {"myapp", "send", "-s"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(3, missing_option, &configuration));
    const char *unknown_option[] = {"myapp", "send", "-x"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(3, unknown_option, &configuration));
    const char *invalid_session[] = {"", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "with space", "abc_1", "{", "abcdefghijklmnopqrstuvwxyz1234567"};
    for (size_t index = 0; index < sizeof(invalid_session) / sizeof(invalid_session[0]); index++)
    {
        const char *arguments[] = {"myapp", "send", "-s", invalid_session[index], "host", "file"};
        TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(6, arguments, &configuration));
    }
    const char *numeric_options[] = {"-w", "-T", "-p"};
    const char *bad_numbers[] = {"", "0", "-1", "+1", " 1", "1x", "999999999999999999999999", "65", "65536", "4294967296"};
    for (size_t option = 0; option < 3; option++)
    {
        for (size_t number = 0; number < sizeof(bad_numbers) / sizeof(bad_numbers[0]); number++)
        {
            if ((option == 1 && (number == 7 || number == 8)) || (option == 2 && number == 7))
            {
                continue;
            }
            const char *arguments[] = {"myapp", "send", "-s", "ok", numeric_options[option], bad_numbers[number], "host", "file"};
            TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(8, arguments, &configuration));
        }
        const char *arguments[] = {"myapp", "recv", "-s", "ok", numeric_options[option], "2", "host", "file"};
        if (option != 2)
        {
            TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(8, arguments, &configuration));
        }
    }
    char overflowing_rate[400];
    memset(overflowing_rate, '9', sizeof(overflowing_rate) - 1);
    overflowing_rate[sizeof(overflowing_rate) - 1] = '\0';
    const char *overflow_arguments[] = {"myapp", "send", "-s", "ok", "-l", overflowing_rate, "host", "file"};
    TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(8, overflow_arguments, &configuration));
    const char *probability_options[] = {"-l", "-c", "-d"};
    const char *bad_rates[] = {"", ".", "0.2.1", "nan", "inf", "1e-1", "-0.1", "0.51", "1", "0.1 ", "0."};
    for (size_t option = 0; option < 3; option++)
    {
        for (size_t rate = 0; rate < sizeof(bad_rates) / sizeof(bad_rates[0]); rate++)
        {
            const char *arguments[] = {"myapp", "send", "-s", "ok", probability_options[option], bad_rates[rate], "host", "file"};
            TEST_ASSERT_EQUAL(TRANSFER_PARSE_ERROR, parse_test_arguments(8, arguments, &configuration));
        }
    }
}

static void test_io_invalid_configuration(void)
{
    TEST_ASSERT_EQUAL(2, run_sender_transfer(NULL));
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(NULL));
    transfer_configuration configuration = create_test_configuration(TRANSFER_SEND, 12345, "/does/not/exist");
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(&configuration));
    configuration.mode = TRANSFER_RECEIVE;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.mode = TRANSFER_SEND;
    configuration.session[0] = '\0';
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    strcpy(configuration.session, "ok");
    configuration.port = 0;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.port = 12345;
    configuration.relay_host = NULL;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.relay_host = "127.0.0.1";
    configuration.file_path = NULL;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.file_path = "/does/not/exist";
    configuration.window_size = 0;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.window_size = 65;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.window_size = 8;
    configuration.timeout_ms = 0;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.timeout_ms = 10;
    configuration.loss_probability = -0.1;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.loss_probability = 0.6;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.loss_probability = 0;
    configuration.corruption_probability = NAN;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.corruption_probability = -0.1;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.corruption_probability = 0.6;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.corruption_probability = 0;
    configuration.duplication_probability = INFINITY;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.duplication_probability = -0.1;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.duplication_probability = 0.6;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.duplication_probability = 0;
    configuration.loss_probability = NAN;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.loss_probability = 0;
    memset(configuration.session, 'a', sizeof(configuration.session));
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
}

static void test_io_sender_success_retries_and_failures(void)
{
    char path[32];
    FILE *file = create_test_file(path);
    uint8_t input[2500];
    memset(input, 0x37, sizeof(input));
    TEST_ASSERT_EQUAL(sizeof(input), fwrite(input, 1, sizeof(input), file));
    TEST_ASSERT_EQUAL(0, fclose(file));
    relay_stub stub = create_relay_stub(STUB_SENDER);
    transfer_configuration configuration = create_test_configuration(TRANSFER_SEND, stub.port, path);
    configuration.relay_host = "localhost";
    configuration.timeout_ms = UINT32_MAX;
    TEST_ASSERT_EQUAL(0, run_sender_transfer(&configuration));
    destroy_relay_stub(&stub);
    configuration.relay_host = "127.0.0.1";
    configuration.timeout_ms = 10;
    stub = create_relay_stub(STUB_RETRY);
    configuration.port = stub.port;
    struct sigaction action = {.sa_handler = handle_test_signal};
    struct sigaction old_action;
    TEST_ASSERT_EQUAL(0, sigaction(SIGALRM, &action, &old_action));
    struct itimerval timer = {.it_value = {.tv_usec = 50000}};
    TEST_ASSERT_EQUAL(0, setitimer(ITIMER_REAL, &timer, NULL));
    TEST_ASSERT_EQUAL(0, run_sender_transfer(&configuration));
    TEST_ASSERT_EQUAL(0, sigaction(SIGALRM, &old_action, NULL));
    destroy_relay_stub(&stub);
    stub = create_relay_stub(STUB_REFUSE);
    configuration.port = stub.port;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    destroy_relay_stub(&stub);
    stub = create_relay_stub(STUB_SILENT);
    configuration.port = stub.port;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    destroy_relay_stub(&stub);
    /* Empty input makes one FIN per fruitless timeout. */
    file = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(file);
    TEST_ASSERT_EQUAL(0, fclose(file));
    stub = create_relay_stub(STUB_NO_ACK);
    configuration.port = stub.port;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    destroy_relay_stub(&stub);
    stub = create_relay_stub(STUB_CLOSE);
    configuration.port = stub.port;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    destroy_relay_stub(&stub);
    configuration.relay_host = "256.256.256.256";
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    TEST_ASSERT_EQUAL(0, unlink(path));
    configuration.file_path = "/does/not/exist/cs425-input";
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    configuration.file_path = "/tmp";
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    file = create_test_file(path);
    TEST_ASSERT_EQUAL(0, ftruncate(fileno(file), PROTOCOL_MAX_INPUT_SIZE + 1));
    TEST_ASSERT_EQUAL(0, fclose(file));
    configuration.file_path = path;
    TEST_ASSERT_EQUAL(2, run_sender_transfer(&configuration));
    TEST_ASSERT_EQUAL(0, unlink(path));
}

static void test_io_receiver_success_and_failures(void)
{
    char path[32];
    FILE *file = create_test_file(path);
    TEST_ASSERT_EQUAL(0, fclose(file));
    relay_stub stub = create_relay_stub(STUB_RECEIVER);
    transfer_configuration configuration = create_test_configuration(TRANSFER_RECEIVE, stub.port, path);
    TEST_ASSERT_EQUAL(0, run_receiver_transfer(&configuration));
    destroy_relay_stub(&stub);
    file = fopen(path, "rb");
    TEST_ASSERT_NOT_NULL(file);
    uint8_t output[4];
    TEST_ASSERT_EQUAL(3, fread(output, 1, sizeof(output), file));
    const uint8_t expected[] = {0, 0xff, 0x37};
    TEST_ASSERT_EQUAL_MEMORY(expected, output, sizeof(expected));
    TEST_ASSERT_EQUAL(0, fclose(file));
    stub = create_relay_stub(STUB_REFUSE);
    configuration.port = stub.port;
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(&configuration));
    destroy_relay_stub(&stub);
    configuration.relay_host = "256.256.256.256";
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(&configuration));
    configuration.relay_host = "127.0.0.1";
    stub = create_relay_stub(STUB_CLOSE);
    configuration.port = stub.port;
    configuration.file_path = "/does/not/exist/cs425-output";
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(&configuration));
    destroy_relay_stub(&stub);
#ifdef __linux__
    stub = create_relay_stub(STUB_RECEIVER);
    configuration.port = stub.port;
    configuration.file_path = "/dev/full";
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(&configuration));
    /* The peer waits for the FIN ACK that must never acknowledge a failed flush. */
    TEST_ASSERT_EQUAL(0, kill(stub.process_id, SIGTERM));
    int status;
    TEST_ASSERT_EQUAL(stub.process_id, waitpid(stub.process_id, &status, 0));
    TEST_ASSERT_TRUE(WIFSIGNALED(status));
#endif
    configuration.file_path = path;
    stub = create_relay_stub(STUB_RECEIVER_IDLE);
    configuration.port = stub.port;
    TEST_ASSERT_EQUAL(2, run_receiver_transfer(&configuration));
    destroy_relay_stub(&stub);
    TEST_ASSERT_EQUAL(0, unlink(path));
}

void run_transfer_io_tests(void)
{
    RUN_TEST(test_cli_defaults_and_options);
    RUN_TEST(test_cli_invalid_arguments);
    RUN_TEST(test_io_invalid_configuration);
    RUN_TEST(test_io_sender_success_retries_and_failures);
    RUN_TEST(test_io_receiver_success_and_failures);
}
/* #endregion Functions */

```

## Scripts Files
Report generated on 10/16/2026 at 06:15:24


---

## End of Report

SHA-256 Hash of the report: 798628c0ea71584a115c540152a6a1b669452af898af7c45d2d4be8c7b2b6c6f

Do not edit the generated report. Any changes will be reported as academic dishonesty

---
## GitHub Info
- GitHub repo name: AndyK111/cs425-p2
- The repository visibility is public.
- The workflow was triggered by AndyK111
