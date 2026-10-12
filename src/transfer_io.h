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
