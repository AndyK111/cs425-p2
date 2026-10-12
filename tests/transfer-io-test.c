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
