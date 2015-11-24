# mycc

This is a compiler for a subset of C language. It's a course project in my university. Now it, the compiler itself, will crash if it is compiled as a 64-bit(x86_64) executable. I'll fix it soon.

It's now can only be compiled and running on 32-bit(x86) architecutre.
To build the compiler:

```
# make a folder for project files and building
mkdir build
cd build
# add flags to cc and ld, specifying the architecutre
cmake -D CMAKE_C_FLAGS:string=-m32 -D CMAKE_EXE_LINKER_FLAGS:string=-m32 ..
make
# then you can run it
./mycc [source file]
```
or
```
mkdir xcode
cd xcode
cmake -G Xcode ..
# then open the project file generated
```
It only rely on standrad library. No other libraries are needed.

It can only accept a single input file and output an assembly source file for it as a result. The assembly source can be assembled by MASM. Don't forget to link it with the I/O library, `lib/mycclib.asm`.


