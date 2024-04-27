# EsetVm2 Bytecode Disassembler

Uncover the secrets of EsetVm2 bytecode with our C++ command-line application.
Developed by Vasil Viazhevich (vasviazh@gmail.com), this tool is perfect for
educational exploration and familiarization with EVM2 file formats.

## Key Features:

- **Performance:** C++ code parser operates on 2, 4, and 8 bytes bytecode data, making it 10 times faster than bit-by-bit parsing.
- **OOP Design:** Implemented a BitStream abstraction for input data within the disassembler class, providing benefits for buffered streamed input.
- **Improve and Test:** Unit tests for the BitStream functionality and samples of disassembler unit testing.
- **Usage:** Run with command line 'eset_vm_disasm input_evm_file_name' to disassemble bytecode (eset_vm_disasm.cpp).
- **Output:** Results conveniently displayed as assembler files in the standard C output stream (can be refactored to write to a file easily).
- **Check Format Errors:** EVM2 file format and bytecode format for errors.
- **Modern C++23:** Must be compiled with "-std=gnu++23" option.
- **Note:** Not for production use without adjustments yet, but a good point to start with. Needs deep testing and additional unit test coverage for disassembler and error check treatment.

## Possible Next Steps (TODOs):

- Further optimize the instruction code matching algorithm (sort and binary search of variable size code), eliminate redundant end-of-code buffer checks, and implement specialized functions for parsing input bytecode based on size and parameters.
- Conduct thorough error checking for all parse errors and provide error descriptions for incorrect input data.
- Implement unit tests for bytecode disassembly, incorporating additional instructions and error checking scenarios.
- Develop unit tests to verify input file data validity.
- Debug and test for Little and big-endian architectures (I have compiled and tested with g++ for amd64 only).

Start your journey into EsetVm2 bytecode today with our disassembler!
