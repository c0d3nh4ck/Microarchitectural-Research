#!/usr/bin/env python3
# coding: utf-8

import subprocess

# num_uops = [0, 1, 10, 11, 13, 133, 15, 16, 17, 18, 2, 20, 22, 25, 27, 3, 31, 34, 35, 36, 4, 5, 57, 6, 8, 89, 9]
num_uops = [15, 16, 17, 18, 2, 20, 22, 25, 27, 3, 31, 34, 35, 36, 4, 5, 57, 6, 8, 89, 9]

a = {}
for num in num_uops:
    # filename = "NEWUOPS-set/NEWUOPS-" + str(num) + "-instruction-set.txt"
    filename = "UOP-instruction-set/UOPS-" + str(num) + "-instruction-set.txt"
    a[num] = []
    with open(filename, 'rb') as f:
        a[num].append(f.readlines())


def bytesToHexStr(hex_asm):
    return ''.join(['{:02x}'.format(byte) for byte in hex_asm])


for num in num_uops:
    print("\n=====%d=====\n" % num)
    
    num_instr = len(a[num][0])
    if num_instr < 5:
        limit = num_instr
    else:
        limit = 5
    
    i = 0
    k = 0
    while i < num_instr:
        # print("\n==>")
        instr_set = []
        bytes_set = []
        
        for instrHex_set in a[num][0][i:i+limit]:
            instr_set.append(str(instrHex_set.split(b' | ')[0].strip())[2:-1])
            bytes_set.append(bytesToHexStr(instrHex_set.split(b' | ')[1][:-1]))
                
        # print(bytes_set)
            
        i += limit
        k += 1
         
        instr_text = "#define INSTR_SET __asm__ volatile(\"mov rcx, 0x424242;mov rbx, 0x414141;mov rdx, 0x434343;push 0x666666;movsd xmm1, [rsp];push 0x444444;movsd xmm0, [rsp];push 0x555555;movsd xmm2, [rsp];"
        instr_text += ';'.join(instr_set) + ';\");'
        # print(instr_text)
        
        with open("instr.h", 'w') as f:
            f.write(instr_text)
        
        compilation = subprocess.run(['gcc', '-w', 'test_run.c', '-masm=intel', '-o', 'test_run'])
        for itr in range(20):
            result = subprocess.run(['sudo', './test_run', str(num), str(k), str(itr)] + bytes_set)
            # print(['./test_run', itr,]+bytes_set)
        
