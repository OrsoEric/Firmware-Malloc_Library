# At_Malloc
A C library that implement malloc and free using a compressed address table to save RAM. Meant for 8b microcontrollers.

# Why
I did this library early in my days of firmware programming. I did this mostly because I wanted to make a malloc function.

# Builtin Malloc
The builtin malloc for Atmel microcontrollers works by having a table of explicit addresses. This is bad because that has a lot of overhead, you can't allocate lots of small vectors.

# How it Works
This library uses a global vector as working memory. it allocate an allocation table at the start of the vector, and return the void * addresses for the malloc call from the end of the vector. The table is compressed using an huffman algorithm based on three bits block. Yes, it take less than a byte to describe a block.

There are algorithms to join fragments together and handle fragmentation, and a policy on which fragments use to allocate new memory. It saves lots of ram, but it uses lots of program memory and it takes a while to execute malloc and free functions as drawback.






