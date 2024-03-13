#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <cstring>
#include <iomanip>

struct Header {
    char magic[8];
    uint32_t codeSize;
    uint32_t dataSize;
    uint32_t initialDataSize;
};

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

const uint8_t valueTypeSize = 2U;
const uint8_t addrSize = 32U;
const uint8_t registerAddrSize = 4U;
const uint8_t constValueSize = 64U;
const uint8_t maxCmdSize = 6;
const std::string codeLablePrefix("addr");

//code buffer to parse
std::vector<uint8_t> codeBuffer;
//current bit in the codeBuffer to parse
uint64_t curParseBitOffset = 0;
//goto (jump) points list, gathering all jump instructions
std::vector<uint32_t> jumpList;

bool readBit(std::vector<uint8_t> bits, uint64_t offset)
{
    uint64_t byteOffset = offset / 8;
    uint64_t bitOffset = 7 - (offset % 8); // little endian bit
    uint64_t mask = 1 << bitOffset;
    return (bits[byteOffset] & mask) != 0;
}

//read instruction code
uint64_t readInstrBits(std::vector<uint8_t> bits, uint64_t offset, uint8_t count)
{
    uint64_t outBits = 0;
    uint8_t countRead = 0;
    while (countRead < count)
    {
        uint64_t bit = readBit(bits, offset + countRead) ? 1U : 0U;
        uint64_t outBitsShifted = outBits << 1;
        outBits = outBitsShifted | bit;
        ++countRead;
    }
    return outBits;
}

//read data little endian bitwise
uint64_t readDataBits(std::vector<uint8_t> bits, uint64_t offset, uint8_t count)
{
    uint64_t outBits = 0;
    uint8_t bitsRead = 0;
    while (bitsRead < count)
    {
        ++bitsRead;
        uint64_t bit = readBit(bits, offset + count - bitsRead) ? 1U : 0U;
        //std::cerr << bit;
        uint64_t outBitsShifted = outBits << 1;
        outBits = outBitsShifted | bit;
    }
    //std::cerr << " " << outBits << std::endl;
    return outBits;
}

// Parse instruction argument
// Return parameter disassembled string
std::string parseArg(const char argType)
{
    switch (argType)
    {
        case 'R': // parse register or memory argument
        {
            std::stringstream disasmStr;
            bool argType = readBit(codeBuffer, curParseBitOffset++);
            if (argType) // parse memory argument
            {
                //parse variable type
                uint64_t argSizeId = readDataBits(codeBuffer, curParseBitOffset, valueTypeSize);
                curParseBitOffset += valueTypeSize;
                disasmStr << dataAccessTypes[argSizeId] << "[";

                // parse variable address
                uint64_t registerIndex = readDataBits(codeBuffer, curParseBitOffset, registerAddrSize);
                curParseBitOffset += registerAddrSize;
                disasmStr << "r" << registerIndex << "]";
            }
            else // parse register argument
            {
                // write address register
                uint64_t registerIndex = readDataBits(codeBuffer, curParseBitOffset, registerAddrSize);
                curParseBitOffset += registerAddrSize;
                disasmStr << "r" << registerIndex;
            }
            return disasmStr.str();
        }
        break;
        case 'C': // parse const value
        {
            uint64_t constArg = readDataBits(codeBuffer, curParseBitOffset, constValueSize);
            curParseBitOffset += constValueSize;
            return std::to_string(constArg);
        }
        break;
        case 'L':
        {
            // write address register
            uint32_t address = readDataBits(codeBuffer, curParseBitOffset, addrSize);
            curParseBitOffset += addrSize;
            jumpList.emplace_back(address);
            return codeLablePrefix + std::to_string(address);
        }
        default:
            return "unknown_argument_format";
    }
}

int main(int argc, char* argv[])
{
    std::string inputFilePath;

    if (argc != 2)
    {
        std::cout << "Usage: eset_vm_disasm in_file.evm" << std::endl;
        return 0;
    }
    else
        inputFilePath = argv[1];

    // open input file
    std::ifstream inputFile(inputFilePath, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    // Read and parse the header
    Header header;
    inputFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!inputFile || (*reinterpret_cast<uint64_t*>(header.magic)
            != *reinterpret_cast<const uint64_t*>("ESET-VM2")))
    {
        std::cerr << "Wrong input file header." << std::endl;
        inputFile.close();
        exit(1);//error
    }

    //print header
    std::cout << ".dataSize " << header.dataSize << std::endl;

    // Read code buffer
    codeBuffer.resize(header.codeSize+1);
    codeBuffer[header.codeSize] = 0;
    inputFile.read(reinterpret_cast<char*>(codeBuffer.data()), header.codeSize);
    if (!inputFile)
    {
        std::cerr << "Error reading file." << std::endl;
        inputFile.close();
        exit(1);//error
    }

    //disassebled instructions list
    std::vector<DisasmInstruct> disasmInstructs;

    // disassemble
    while (curParseBitOffset < (codeBuffer.size()-1)*8)
    {
        //instruction with parameters disassembly
        std::stringstream disasmStr;

        //add new parsed instruction
        DisasmInstruct disasmInstruct(curParseBitOffset);

        uint8_t instrCode = readInstrBits(codeBuffer, curParseBitOffset, maxCmdSize);

        auto isCodeMatching = [instrCode](const InstrCode & code)
        {
            //uint8_t bitMask = ~((uint8_t)0xff << code.codeLength);
            uint8_t cmdShift = maxCmdSize - code.codeLength;
            return code.code == (instrCode >> cmdShift);
        };

        //find operation code
        auto instrCodeIt = std::find_if(instrCodes.begin(), instrCodes.end(), isCodeMatching);
        if (instrCodeIt != instrCodes.end())
        {
            disasmStr << instrCodeIt->name;
            curParseBitOffset += instrCodeIt->codeLength;
        }
        else
        {
            std::cerr << "Unknown_instruction" << std::endl;
            exit(1);//format error
        }

        //parse instruction arguments
        if (instrCodeIt->args.size())
        {
            //first element does not includes ',', output it a bit another way
            disasmStr << " " << parseArg(instrCodeIt->args[0]);
            for (uint8_t argIndex = 1; argIndex < instrCodeIt->args.size(); ++argIndex)
            {
                disasmStr << ", " << parseArg(instrCodeIt->args[argIndex]);
            }
        }
        if (curParseBitOffset <= (header.codeSize)*8)
        {
            disasmInstruct.instrDisasmString = disasmStr.str();
            disasmInstructs.emplace_back(disasmInstruct);
        }
        //aline code offset
        if (curParseBitOffset >= header.codeSize*8)
        {
            curParseBitOffset = header.codeSize*8;
            break;
        }
    }
    /////////////////////////////////////////
    // read initial data buffer
    if (header.initialDataSize)
    {
        std::vector<uint8_t> dataBuffer;
        dataBuffer.resize(header.initialDataSize);
        inputFile.read(reinterpret_cast<char*>(dataBuffer.data()), dataBuffer.size());
        if (!inputFile)
        {
            std::cerr << "Error reading input file initial data." << std::endl;
            inputFile.close();
            exit(1);//error
        }

        //disassamble binary data to HEX string lines
        std::stringstream dataBufStrStream;
        dataBufStrStream << ".data";
        for (size_t i = 0; i < dataBuffer.size(); i++)
        {
            char delim = (i % 16 == 0) ? '\n' : ' ';
            dataBufStrStream << delim << std::hex << std::setfill('0')
                             << std::setw(2) << (int)dataBuffer[i];
        }
        std::cout << dataBufStrStream.str() << std::endl;
    }

    // sort array for using binary search there later
    std::sort(jumpList.begin(), jumpList.end());

    // print instructions
    std::cout << ".code" << std::endl;
    for (auto & instr : disasmInstructs)
    {
        // check we need code address reference here
        if (std::binary_search(jumpList.begin(), jumpList.end(), instr.address))
            std::cout << codeLablePrefix << instr.address << ":" << std::endl;
        std::cout << "\t" << instr.instrDisasmString << std::endl;
    }

    return 0;
}
