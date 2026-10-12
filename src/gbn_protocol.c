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
