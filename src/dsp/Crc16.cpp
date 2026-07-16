#include "Crc16.h"

namespace psk::dsp {

std::uint16_t Crc16::compute(const unsigned char *data, std::size_t length)
{
    std::uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < length; ++i) {
        crc ^= static_cast<std::uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u) {
                crc = static_cast<std::uint16_t>((crc << 1) ^ 0x1021u);
            } else {
                crc = static_cast<std::uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

std::uint16_t Crc16::compute(const std::string &data)
{
    return compute(reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

} // namespace psk::dsp
