#pragma once
#include "bit_istream.hpp"

class EvmParser
{
public:
    EvmParser(BitStream * bitStream);

    enum class ErrorStatus
    {
        NoError = 0,
        FormatError
    };

    ErrorStatus parse(std::ostream &outputStream);

    ErrorStatus getErrorStatus() {return m_errorStatus;}

private:
    std::string parseArg(const char argType);
    uint64_t readDataBits(uint8_t count);
private:
    //goto (jump) points list, collecting all jump instructions address
    std::vector<uint32_t> m_jumpList;

    BitStream *m_pBitStream;
    ErrorStatus m_errorStatus;
};