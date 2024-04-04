#include "bit_istream.hpp"
#include <cassert>
#include <iostream>
#include <cstdint>
#include <cassert>

int main(int argc, char* argv[])
{
    // check read big endian bitwise read
    {
        const std::uint8_t buf[] = {0x11, 0x02, 0xff};
        BitStream bitStream(buf, sizeof(buf));

        assert(bitStream.getBits<1>() == 0);
        assert(bitStream.getBits<3>() == 1);
        assert(bitStream.getBits<3>() == 0);//+7
        assert(bitStream.getBits<3>() == 0b100);//+10
        assert(bitStream.getBits<8>() == 0b00001011);//+18
        assert(bitStream.getBits<6>() == 0b111111);//+24
        assert(!bitStream.eof());
        assert(bitStream.getBits<1>() == 0);//+25
        assert(bitStream.eof());

        bitStream.reset();
        assert(bitStream.getBits<8>() == 0x11);
        assert(bitStream.getBits<8>() == 0x02);
        assert(bitStream.getBits<4>() == 0x0f);
        assert(bitStream.getBits<4>() == 0x0f);
        assert(!bitStream.eof());
        assert(bitStream.getBits<8>() == 0);
        assert(bitStream.eof());

        bitStream.reset();
        assert(bitStream.getBits<8>() == 0x11);
        assert(bitStream.getBits<8>() == 0x02);
        assert(bitStream.getBits<4>() == 0x0f);
        assert(bitStream.getBits<8>() == 0x00);
        assert(bitStream.eof());
    }

    // bytes data read
    {
        const std::uint8_t buf[] = {0x01, 0xff, 0x00, 0x04};
        BitStream bitStream(buf, sizeof(buf));
        assert(bitStream.getInt<uint8_t>() == 0x80);
        assert(bitStream.getInt<uint16_t>() == 0x00ff);
        assert(!bitStream.eof());
        assert(bitStream.getInt<uint16_t>() == 0x00);
        assert(bitStream.eof());
    }
    // bytes data read
    {
        const std::uint8_t buf[] = {0b10100000, 0b00000011, 0b00000011, 0b00000011};
        BitStream bitStream(buf, sizeof(buf));
        assert(bitStream.getBits<2>() == 0b10);
        assert(bitStream.getInt<uint8_t>() == 0b00000001);
        assert(bitStream.getInt<uint16_t>() == 0b0011000000110000);
        assert(!bitStream.eof());
        assert(bitStream.getBits<6>() == 0b000011);
        assert(!bitStream.eof());
        assert(bitStream.getInt<uint64_t>() == 0);
        assert(bitStream.eof());
    }

    return 0;
}
