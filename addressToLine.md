# How to interpret Dwarf debugging info

## .debug_line section

`readelf --debug-dump=decodedline ~/Documents/personalProjects/debugger/build/hello.tsk `

^ This command is super good for translating from line numbers to addresses. It doesn't tell you what the symbols are
but just converts the line number to an address in memory.

This info is also in the debug_line part of dwarfdump (you just need to scroll down to find the starting line numbers):
`dwarfdump ~/Documents/personalProjects/debugger/build/hello.tsk | less`


## .debug_info section

This section has information about the variables in the program and has the line numbers where each variable is defined.