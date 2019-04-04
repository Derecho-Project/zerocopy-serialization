### What is this?
A bit of code I wrote to observe the resulting behavior when an instance of an STL container using a custom allocator is copied from one region to another and its internal pointers are rewritten. The purpose was to make sure the allocator we are writing for Derecho will not encounter any fundamental problems in this area.

### To build:
```
$ mkdir debug && cd debug
$ cmake ..
$ make
```

### To disable ASLR:
`rarun2 aslr=no system=<executable>`
