global _start

section .data
    msg db "asm: hello, speed world!", 10  ; The message to print, followed by a newline (10)
    msg_len equ $ - msg         ; Calculate the length of the string

section .text
_start:
    ; Initialize our two numbers
    mov rbx, 0                  ; First number (counter starts at 0)
    mov r12, 5000000            ; Second number (target limit)

.loop_start:
    ; 1. Print "hello, world!"
    mov rax, 1                  ; Syscall ID for sys_write (1)
    mov rdi, 1                  ; File descriptor 1 (stdout)
    mov rsi, msg                ; Memory address of the message
    mov rdx, msg_len            ; Length of the message in bytes
    syscall                     ; Trigger the system call

    ; 2. Increment the zero
    inc rbx                     ; Add 1 to our counter in rbx

    ; 3. Compare the two numbers
    cmp rbx, r12                ; Compare counter (rbx) with limit (r12)

    ; 4. Loop until they are the same
    jne .loop_start             ; Jump back to .loop_start if Not Equal

.exit:
    ; Exit the program cleanly once the loop finishes
    mov rax, 60                 ; Syscall ID for sys_exit (60)
    xor rdi, rdi                ; Set exit code to 0
    syscall                     ; Trigger the system call
