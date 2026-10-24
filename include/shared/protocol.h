/**
 * FishFeeder — shared ESP-NOW packet format (UI node <-> relay node).
 * 16 bytes, checksummed, unencrypted (payload is a harmless feed command,
 * magic + checksum reject garbage heard on a shared channel).
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

#define FF_PROTO_MAGIC 0xCA7FU  // "cat fish"
#define FF_PROTO_VERSION 1

// Commands (UI node -> relay node) and replies (relay node -> UI node)
enum ff_cmd : uint8_t {
  FF_CMD_PING = 0x01,  // heartbeat, value = echo of sender's seq
  FF_CMD_FEED = 0x02,  // value = pulse duration in ms
  FF_CMD_ACK = 0x7F,   // reply: value = echoed seq of the original packet
};

// What triggered the feed (recorded in the feed history)
enum ff_source : uint8_t {
  FF_SRC_MANUAL = 0x01,
  FF_SRC_SCHEDULE = 0x02,
};

struct ff_packet {
  uint16_t magic;    // FF_PROTO_MAGIC
  uint8_t version;   // FF_PROTO_VERSION
  uint8_t cmd;       // ff_cmd
  uint32_t seq;      // incrementing sequence number
  uint32_t value;    // cmd payload (duration ms / echoed seq)
  uint8_t source;    // ff_source
  uint8_t rssi;      // filled by the receiver on ACK (abs value, 0 = n/a)
  uint16_t reserved; // zero
  uint16_t checksum; // 16-bit sum of all preceding bytes
} __attribute__((packed));

static_assert(sizeof(ff_packet) == 16, "ff_packet must stay 16 bytes");

inline uint16_t ff_checksum(const ff_packet &p) {
  const uint8_t *b = reinterpret_cast<const uint8_t *>(&p);
  uint32_t sum = 0;
  for (size_t i = 0; i < offsetof(ff_packet, checksum); i++)
    sum += b[i];
  return (uint16_t)(sum & 0xFFFF);
}

inline void ff_pack(ff_packet &p, uint8_t cmd, uint32_t seq, uint32_t value,
                    uint8_t source) {
  p.magic = FF_PROTO_MAGIC;
  p.version = FF_PROTO_VERSION;
  p.cmd = cmd;
  p.seq = seq;
  p.value = value;
  p.source = source;
  p.rssi = 0;
  p.reserved = 0;
  p.checksum = ff_checksum(p);
}

inline bool ff_unpack(ff_packet &p) {
  if (p.magic != FF_PROTO_MAGIC || p.version != FF_PROTO_VERSION)
    return false;
  return p.checksum == ff_checksum(p);
}
