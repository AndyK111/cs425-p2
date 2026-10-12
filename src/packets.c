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
