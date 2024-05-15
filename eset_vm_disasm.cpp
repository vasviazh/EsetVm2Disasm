#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <chrono>
#include "bit_istream.hpp"
#include "evm_parser.hpp"

struct Header {
    char magic[8];
    uint32_t codeSize;
    uint32_t dataSize;
    uint32_t initialDataSize;
};

int main(int argc, char* argv[])
{
    std::string inputFilePath;

    std::stringstream outAsmCodeStream;

    std::string outFileName;
    if (argc == 2)
    {
        inputFilePath = argv[1];
    }
    else
    {
        std::cout << "Usage: eset_vm_disasm in_file.evm" << std::endl;
        return 0;
    }

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
    outAsmCodeStream << ".dataSize " << header.dataSize << std::endl;

    // Read code buffer
    std::vector<uint8_t> codeBuffer(header.codeSize);
    inputFile.read(reinterpret_cast<char*>(codeBuffer.data()), header.codeSize);
    if (!inputFile)
    {
        std::cerr << "Error reading code buffer from file." << std::endl;
        inputFile.close();
        exit(1);//error
    }

    auto time_begin = std::chrono::steady_clock::now();

    /////////////////////////////////////////
    // read initial data buffer
    if (header.initialDataSize)
    {
        std::vector<uint8_t> dataBuffer(header.initialDataSize);
        inputFile.read(reinterpret_cast<char*>(dataBuffer.data()), dataBuffer.size());
        if (!inputFile)
        {
            std::cerr << "Error reading input file initial data." << std::endl;
            inputFile.close();
            exit(1);//error
        }

        //disassamble binary data to HEX string lines
        outAsmCodeStream << ".data";
        std::ios_base::fmtflags streamFlags(outAsmCodeStream.flags());
        outAsmCodeStream << std::hex;
        for (size_t i = 0; i < dataBuffer.size(); i++)
        {
            char delim = (i % 16 == 0) ? '\n' : ' ';
            outAsmCodeStream << delim << std::setfill('0')
                             << std::setw(2) << (int)dataBuffer[i];
        }
        outAsmCodeStream.flags(streamFlags);
        outAsmCodeStream << std::endl;
    }

    // disassemble code buffer
    BitStream codeBitStream(codeBuffer.data(), header.codeSize);
    EvmParser codeParser(&codeBitStream);
    codeParser.parse(outAsmCodeStream);
    if (codeParser.getErrorStatus() != EvmParser::ErrorStatus::NoError)
    {
        std::cerr << "Parse error" << std::endl;
        exit((int)codeParser.getErrorStatus());
    }

    auto time_end = std::chrono::steady_clock::now();
    
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_begin);
    std::cerr << "Parse time elapsed: " << elapsed_ms.count() << std::endl;

    std::cout << outAsmCodeStream.str();
    return 0;
}
