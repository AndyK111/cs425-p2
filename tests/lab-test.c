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
