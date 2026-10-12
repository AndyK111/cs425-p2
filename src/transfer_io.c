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
