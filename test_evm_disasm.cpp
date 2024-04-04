#include "evm_parser.hpp"
#include <iostream>
#include <cstdint>

class BitStreamTest
{
public:
    BitStreamTest(BitStream & bitStream)
        : m_bitStream(bitStream) {}

    template<class T>
    T checkRead(size_t bits, T expectedResult, bool eofExpected = false)
    {
        std::cout << "Test read " << bits << " bits to " << sizeof(T)
                  << " bytes integer (expectedResut="
                  << std::bitset<sizeof(T)*8>(expectedResult)
                  << "): ";
        T result = m_bitStream.getBits<T>(bits);

        if (eofExpected != m_bitStream.eof())
        {
            std::cout << "Fail! EOF flag not valid - expected "
                      << eofExpected << ", got " << m_bitStream.eof()
                      << std::bitset<sizeof(T)*8>(result) << std::endl;
            m_bFail |= true;
        }
        else if (result == expectedResult)
        {
            std::cout << "Success!" << std::endl;
        }
        else
        {
            std::cout << "Fail! Got value: "
                      << std::bitset<sizeof(T)*8>(result) << std::endl;
            m_bFail |= true;
        }
        return result;
    }
    BitStream &m_bitStream;
    bool m_bFail = false;
};

int main(int argc, char* argv[])
{
    const std::uint8_t buf[] = { 0b01000101, 0b00010101, 0b00011000,};
                                // 0b01000101, 0b00010101, 0b00011000,
                                // 0b01000101, 0b00010101, 0b00011000,
                                // 0b01000101, 0b00010101, 0b00011000,
                                // 0b01000101, 0b00010101, 0b00011000,
                                // 0b01000101, 0b00010101, 0b00011000};
    BitStream bitStream(buf, sizeof(buf));
    EvmParser parser(&bitStream);
    parser.parse(std::cout);
}
