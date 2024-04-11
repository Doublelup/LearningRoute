define hook-quit
    kill
end

set pagination off
target remote localhost:1234
file build/kernel-x86_64-qemu.elf
break main
break trap64.S:__am_iret
break trap64.S:trap
layout src
continue
