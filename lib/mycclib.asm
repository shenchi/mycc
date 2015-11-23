;  i/o library for mycc

_myccout proc
    push ebp
    mov ebp, esp
    sub esp, 20h

    xor eax, eax
    mov [esp], al

    mov eax, [ebp+8]

    cmp eax, 0
    jnz _@myccout@1

    mov eax, [ebp+12]
    invoke ltoa, eax, esp
    jmp _@myccout@0
    
_@myccout@1:

    cmp eax, 1
    jnz _@myccout@2

    mov eax, [ebp+12]
    mov [esp], al
    xor eax, eax
    mov [esp+1], al
    jmp _@myccout@0
    
_@myccout@2:

    cmp eax, 2
    jnz _@myccout@3

    fld dword ptr [ebp + 12]
    fst qword ptr [esp + 20]

    lea eax, [esp+20]
    lea ebx, [esp]

    invoke FloatToStr, qword ptr [eax], ebx

    jmp _@myccout@0
    
_@myccout@3:

    cmp eax, 3
    jnz _@myccout@0

    mov eax, [ebp+12]

    invoke StdOut, eax
    leave
    ret 8

_@myccout@0:

    invoke StdOut, esp

    leave
    ret 8
_myccout endp


_myccin proc
    push ebp
    mov ebp, esp
    sub esp, 32

_@myccin@0:
    lea eax, [esp]
    invoke StdIn, eax, 20

    mov eax, [ebp+8]

    cmp eax, 0
    jnz _@myccin@1

    lea ebx, [esp]
    invoke atol, ebx

    mov ebx, [ebp + 12]
    mov [ebx], eax
    jmp _@myccin@3

_@myccin@1:

    cmp eax, 1
    jnz _@myccin@2

    xor eax, eax
    mov al, [esp]
    cmp eax, 0
    je  _@myccin@err
    cmp eax, 7fh
    ja  _@myccin@err

    mov ebx, [ebp+12]
    mov [ebx], eax

    jmp _@myccin@3
    
_@myccin@2:

    cmp eax, 2
    jnz _@myccin@3

    lea ebx, [esp]
    lea eax, [esp+20]
    invoke StrToFloat, ebx, eax

    fld qword ptr [esp+20]
    mov ebx, [ebp+12]
    fst dword ptr [ebx]

_@myccin@3:

    leave
    ret 8

_@myccin@err:
    jmp _@myccin@0
_myccin endp


