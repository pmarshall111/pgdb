# Peter's General(ish) Debugger

## To run

```
make clean && make build
make debug
```

## How it works

This project uses the `fork()`, `execl()` and `ptrace()` system calls to start the program we want to debug in a new process.

`ptrace(PTRACE_TRACEME)` is the magic function here. When called in a child process it allows a parent process to trace it. As the parent process, once this is called we are now allowed to read and write to the internal registers and data of the child process.

To set a breakpoint, we first need to know an address we want to stop on. To do this, we can use the debug information (created when compiling the process with the `-g` flag. I.e. `g++ -g`) to find the memory address of a line or function. To view the debug information for a task, use the `dwarfdump` utility. Note that the addresses in the debug information only define an offset from the start of the memory area that contains the code, not the actual memory address we want to stop on. We therefore need to add the address we found in the debug information to the start address of where the code is mapped in the child process. Due to [address space layout randomisation](https://en.wikipedia.org/wiki/Address_space_layout_randomization), the start of the code section will be different every time the process runs. However, we can just look at the `/proc/<pid>/maps` file to find where the code was mapped in add this to our address from the debug information to find the address to set our breakpoint.

To actually set a breakpoint, we overwrite an assembly instruction at the address to the [int3](https://en.wikipedia.org/wiki/INT_(x86_instruction)) instruction, which in X86 creates a software interrupt. Note that since we've overwritten an original assembly instruction, if we didn't do anything else the program would not be the same as when running without a debugger. Once we hit a breakpoint, we must therefore:
- step back 1 instruction
- replace the int3 with the original instruction
- step forward 1 instruction to run the original instruction
- again overwrite the original instruction with int3, so the breakpoint will be triggered if we hit this logic again.

## Other cool things

There's a macro to add the name of the process to any fprintf calls in [fprintfAddExecutableName.h](./fprintfAddExecutableName.h). We can therefore see which process is doing which prints:

```
[pgdb] debugger started. Child pid: 52013
[pgdb] target started. will run 'build/hello.tsk'
[hello.tsk] Random number: 1804289383
[pgdb] Stopped
```

## Helpful sources

https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/

This repo was made using the help of the above tutorial