#include "evm_parser.hpp"
#include <istream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <bit>

struct InstrCode
{
    std::string name;
    uint8_t code;
    uint8_t codeLength;
    std::string args;
};

struct DisasmInstruct
{
    DisasmInstruct(uint32_t _address)
        : address(_address)
    { }
 
    uint32_t address;
    std::string instrDisasmString;
};

typedef std::vector<InstrCode> InstrCodesList;

const InstrCodesList instrCodes = {

    {"mov", 0b000, 3, "RR"},       // 000 mov r1, r2
    {"loadConst", 0b001, 3, "CR"}, // 001 loadConst const, r1

    {"add", 0b010001, 6, "RRR"}, // 010001 add r1, r2, r3
    {"sub", 0b010010, 6, "RRR"}, // 010010 sub r1, r2, r3
    {"div", 0b010011, 6, "RRR"}, // 010011 div r1, r2, r3
    {"mod", 0b010100, 6, "RRR"}, // 010100 mod r1, r2, r3
    {"mul", 0b010101, 6, "RRR"}, // 010101 mul r1, r2, r3

    {"compare", 0b01100, 5, "RRR"}, // 01100 compare r1, r2, r3 -> (-1, 0, 1)
    {"jump", 0b01101, 5, "L"}, // 01101 jump code-offset
    {"jumpEqual", 0b01110, 5, "LRR"}, // 01110 jumpEqual code-offset, r1, r2

    {"read", 0b10000, 5, "RRRR"}, // 10000 read r-offset, r-count, r-outputaddress, r-readsize
    {"write", 0b10001, 5, "RRR"}, // 10001 write r-offset, r-count, r-outputaddress

    {"consoleRead", 0b10010, 5, "R"}, // 10010 consoleRead r1
    {"consoleWrite", 0b10011, 5, "R"}, // 10011 consoleWrite r1

    {"createThread", 0b10100, 5, "LR"},
    {"joinThread", 0b10101, 5, "R"},
    {"hlt", 0b10110, 5, ""},
    {"sleep", 0b10111, 5, "R"},

    {"call", 0b1100, 4, "L"}, // 1100 call code-offset
    {"ret", 0b1101, 4, ""}, // 1101 ret

    {"lock", 0b1110, 4, "R"}, // 1110 lock index
    {"unlock", 0b1111, 4, "R"}, // 1111 unlock index
};

const std::vector<std::string> dataAccessTypes = { "byte", "word", "dword", "qword" };

constexpr uint8_t valueTypeSize = 2U;
constexpr uint8_t addrSize = 32U;
constexpr uint8_t registerAddrSize = 4U;
constexpr uint8_t constValueSize = 64U;
constexpr uint8_t minCmdBits = 3;
constexpr uint8_t maxCmdBits = 6;
const std::string codeLablePrefix("addr");

EvmParser::EvmParser(BitStream * bitStream)
    : m_pBitStream(bitStream)
    , m_errorStatus(ErrorStatus::NoError)
{

}

// Parse instruction argument
// Return parameter disassembled string
std::string EvmParser::parseArg(const char argType)
{
    std::stringstream disasmStr;
    switch (argType)
    {
        case 'R': // parse register or memory argument
        {
            uint8_t argTypeIsMemory = m_pBitStream->getBits<1>();
            if (argTypeIsMemory) // parse memory argument
            {
                //parse variable type
                uint8_t argSizeId = m_pBitStream->getBits<valueTypeSize, true>();
                disasmStr << dataAccessTypes[argSizeId];

                // parse redister id
                // register id is little endian bitwise like address and constant
                uint16_t registerIndex = m_pBitStream->getBits<registerAddrSize, true>();
                disasmStr << "[r" << registerIndex << "]";
            }
            else // parse register argument
            {
                // write address register
                uint8_t registerIndex = m_pBitStream->getBits<registerAddrSize, true>();
                disasmStr << "r" << (uint16_t)registerIndex;
            }
            return disasmStr.str();
        }
        break;
        case 'C': // parse const value
        {
            disasmStr << std::hex << "0x" << m_pBitStream->getInt<uint64_t>();
            return disasmStr.str();
        }
        break;
        case 'L':
        {
            uint32_t address = m_pBitStream->getInt<uint32_t>();
            m_jumpList.emplace_back(address);
            return codeLablePrefix + std::to_string(address);
        }
        default:
            return "unknown_argument_format";
    }
}

EvmParser::ErrorStatus EvmParser::parse(std::ostream &outputStream)
{
    //disassebled instructions list
    std::vector<DisasmInstruct> disasmInstructs;
    
    while (!m_pBitStream->eof())
    {
        //instruction with parameters disassembly
        std::stringstream disasmStr;

        //add new parsed instruction
        DisasmInstruct disasmInstruct(m_pBitStream->tellg());

        uint8_t instrCodeBits = minCmdBits;
        uint8_t instrCode = m_pBitStream->getBits<minCmdBits>();

        auto isCodeMatching = [&instrCode, &instrCodeBits](const InstrCode & code)
        {
            return (code.code == instrCode) && (instrCodeBits == code.codeLength);
        };

        //find operation code
        //TODO sort and use std::lower_bound()
        for (size_t i=minCmdBits; i<=maxCmdBits; i++)// (!instrCodeIt && !m_pBitStream->eof() && (instrCodeBits <= maxCmdBits))
        {
            if (m_pBitStream->eof())
                break;
            if (instrCodeBits > maxCmdBits)
            {
                std::cerr << "Unknown_instruction code " << instrCode << std::endl;
                m_errorStatus = ErrorStatus::FormatError;
                return m_errorStatus;
            }
            //find instruction
            auto instrCodeIt = std::find_if(instrCodes.begin(), instrCodes.end(), isCodeMatching);
            if (instrCodeIt != instrCodes.end())
            {
                disasmStr << instrCodeIt->name;

                //parse instruction arguments
                //TODO: add eof check
                if (instrCodeIt->args.size())
                {
                    //first element does not includes ',', output it a bit another way
                    std::string instrArg = parseArg(instrCodeIt->args[0]);
                    disasmStr << " " << instrArg;
                    for (uint8_t argIndex = 1; argIndex < instrCodeIt->args.size(); ++argIndex)
                    {
                        disasmStr << ", " << parseArg(instrCodeIt->args[argIndex]);
                    }
                }
                if (!m_pBitStream->eof())
                {
                    disasmInstruct.instrDisasmString = disasmStr.str();
                    disasmInstructs.emplace_back(disasmInstruct);
                }
                break;
            }
            else
            {
                instrCode <<= 1;
                instrCode += m_pBitStream->getBits<1>();
                instrCodeBits++;
            }
        };
    }

    // sort array for using binary search there later
    std::sort(m_jumpList.begin(), m_jumpList.end());

    // print instructions
    outputStream << ".code" << std::endl;
    for (auto & instr : disasmInstructs)
    {
        // check we need code address reference here
        if (std::binary_search(m_jumpList.begin(), m_jumpList.end(), instr.address))
            outputStream << codeLablePrefix << instr.address << ":" << std::endl;
        outputStream << "\t" << instr.instrDisasmString << std::endl;
    }

    return ErrorStatus::NoError;
}