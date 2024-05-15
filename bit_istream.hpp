#pragma once
#include <iostream>
#include <vector>
#include <bitset>
#include <cstdlib>
#include <cstdint>
#include <bit>


class BitStream
{
public:
    BitStream(const std::uint8_t *buf, size_t bufSizeBytes);

    template <size_t NUM_BITS, bool swapBits = false>
    uint8_t getBits()
    {
        static_assert((NUM_BITS>0) && (NUM_BITS<=8), "Bits count of getBits() must be in the range [1, 8]");

        //check for end of buffer
        if (m_byteOffet + (m_bitOffset + NUM_BITS - 1) / 8 >= m_bufSizeBytes)
        {
            m_status = Status::STATUS_EOF;
            return 0; // out of buffer
        }

        uint8_t result = m_buf[m_byteOffet];
        result <<= m_bitOffset;
        if (m_bitOffset + NUM_BITS <= 8)
        {
            result >>= (8-NUM_BITS);
        }
        else
        {
            result >>= 8-NUM_BITS;
            result += m_buf[m_byteOffet+1] >> (8-(m_bitOffset+NUM_BITS) % 8);
        }
        m_byteOffet += (m_bitOffset + NUM_BITS) / 8;
        m_bitOffset = (m_bitOffset + NUM_BITS) % 8;

        if constexpr (swapBits)
        {
            result = reverse_bits8(result);
            result >>= (8 - NUM_BITS);
        }

        return result;
    };

    template<class T>
    T getInt()
    {
        size_t numBits = sizeof(T)*8;

        //check for end of buffer
        if (m_byteOffet + sizeof(T) + (m_bitOffset+7) / 8 > m_bufSizeBytes)
        {
            m_status = Status::STATUS_EOF;
            return 0; // out of buffer
        }

        const uint8_t *byteBuf = &(m_buf[m_byteOffet]);
        // extract data
        T result = *reinterpret_cast<const T*>(byteBuf);
        if (std::endian::native == std::endian::little)
            result = std::byteswap(result);
        result <<= m_bitOffset;
        result += byteBuf[sizeof(T)] >> (8 - m_bitOffset);
        if (std::endian::native == std::endian::little)
            result = std::byteswap(result);
        result = reverse_bits_in_bytes(result);

        m_byteOffet += (m_bitOffset + numBits) / 8;
        m_bitOffset = (m_bitOffset + numBits) % 8;

        return result;
    }

    void reset()
    {
        m_bitOffset = 0;
        m_byteOffet = 0;
        m_status = Status::STATUS_NO_ERROR;
    }

    bool eof() { return m_status == Status::STATUS_EOF; }

    enum class Status
    {
        STATUS_NO_ERROR = 0,
        STATUS_EOF = 1
    };

    size_t tellg()
    {
        return (m_byteOffet * 8) + m_bitOffset;
    }
    
private:
    // Bit Twiddling Hacks By Sean Eron Anderson
    // Reverse the bits in a byte with 4 operations
    uint8_t reverse_bits8(uint8_t b);

    template <class T>
    T reverse_bits_in_bytes(T number)
    {
        T outNumber = 0;
        for (size_t i = 0; i<sizeof(T); i++)
        {
            T nextByte = reverse_bits8((uint8_t)number & 0xff);
            outNumber += nextByte;
            outNumber = std::rotr(outNumber, 8);
            number >>= 8;
        }
        return outNumber;
    }

    Status m_status;

    const std::uint8_t *m_buf;
    const size_t m_bufSizeBytes;

    size_t m_byteOffet = 0;
    size_t m_bitOffset = 0;
};
