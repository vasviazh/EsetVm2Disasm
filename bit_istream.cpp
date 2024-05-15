#include "bit_istream.hpp"
#include <limits>

BitStream::BitStream(const std::uint8_t *buf, size_t bufSizeBytes)
    : m_status(Status::STATUS_NO_ERROR)
    , m_buf(buf)
    , m_bufSizeBytes(bufSizeBytes)
    {
    }

// Bit Twiddling Hacks By Sean Eron Anderson
// Reverse the bits in a byte with 4 operations
uint8_t BitStream::reverse_bits8(uint8_t b)
{
    return ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
}
