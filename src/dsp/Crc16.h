#pragma once

#include <cstdint>
#include <string>

namespace psk::dsp {

// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no final
// xor) - a standard, widely-implemented CRC variant, not a novel design.
// Used to detect (not correct) frame corruption that gets past the FEC
// decoder, before accepting a decoded payload as valid.
class Crc16 {
public:
    static std::uint16_t compute(const std::string &data);
    static std::uint16_t compute(const unsigned char *data, std::size_t length);
};

} // namespace psk::dsp
