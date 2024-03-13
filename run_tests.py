import os

def compare_binary_files(file1_path, file2_path):
    try:
        with open(file1_path, 'rb') as file1, open(file2_path, 'rb') as file2:
            while True:
                byte1 = file1.read(1)
                byte2 = file2.read(1)

                if byte1 != byte2:
                    print("Files are different.")
                    return False

                if not byte1:
                    break  # Reached end of file

            print("Files are identical.")
            return True
    except FileNotFoundError:
        print("One or both files not found.")
    return False


samples_dir = 'task/samples/'
compiled_samples_dir = 'task/samples/precompiled/'

test_names = ["crc", "fibonacci_loop", "lock", "math", "memory",
              "multithreaded_file_write", "philosophers", "pseudorandom",
              "threadingBase", "xor-with-stack-frame", "xor"]

all_tests_passed = True

for test_name in test_names:
    print('Test run: ' + test_name)
    # easm_file = samples_dir + test_name + '.easm'
    test_evm_file = compiled_samples_dir + test_name + '.evm'
    disassembled_easm_file = samples_dir + test_name + '_disassembly.easm'
    out_evm_file = compiled_samples_dir + test_name + '_disassembly.evm'

    # decompile binary EVM2 (EsetVm2 bytecode) file
    cmd = 'eset_vm_disasm ' + test_evm_file + ' > ' + disassembled_easm_file
    print(cmd)
    os.system(cmd)

    # compile disassembled EASM file to binary EVM file
    cmd = 'python compiler.py ' + disassembled_easm_file + ' ' + out_evm_file
    print(cmd)
    res = os.system(cmd)
    if res:
        print('FAILED to compile EASM!')
        all_tests_passed = False
        continue

    # check test passed
    res = compare_binary_files(out_evm_file, test_evm_file)
    all_tests_passed = all_tests_passed & res

if all_tests_passed:
    print("Test success - all tests passed!")
else:
    print("FAILED tests(((")