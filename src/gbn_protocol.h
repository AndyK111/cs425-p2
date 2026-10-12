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
