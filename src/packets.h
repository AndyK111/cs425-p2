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
