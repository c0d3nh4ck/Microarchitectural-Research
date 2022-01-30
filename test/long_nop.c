int main() 
{
    for (int i=0; i<50000; i++)
    {
        __asm__ volatile(
                ".intel_syntax noprefix;"
                "nop;"
                "xchg ax, ax;"
                "nop dword ptr [rax];"
                "nop dword ptr [rax+0x0];"
                );
    }
}

                
