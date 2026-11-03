/* SPDX-License-Identifier: GPL-2.0-only */
/* Private DOS UI oracle. Relocated far calls use the checksum-verified MAP. */
.code16
.intel_syntax noprefix
.global _start
_start:
 push ds
 push cs
 pop ds
 mov dx,offset filename
 xor cx,cx
 mov ah,0x3c
 int 0x21
 jc fail
 mov cs:[handle],ax
 pop ds
 .byte 0x9a
 .word 0x2e09
setupseg: .word 0x6a5
message_loop:
 .byte 0x9a
 .word 0x2cf7
clearseg: .word 0x6a5
 mov ax,cs:[number]
 mov word ptr ds:[0xa046],200
 cmp ax,9
 jb first_bank
 sub ax,9
 mov word ptr ds:[0xa046],160
first_bank:
 mov bx,80
 mul bx
 mov bx,ds
 sub bx,0x16ea
 push bx
 push ax
 .byte 0x9a
 .word 0x3960
messageseg: .word 0x6a5
 add sp,4
 call dump
 inc word ptr cs:[number]
 cmp word ptr cs:[number],18
 jb message_loop
 mov word ptr cs:[number],0
disk_loop:
 .byte 0x9a
 .word 0x2cf7
clear2seg: .word 0x6a5
 push word ptr cs:[number]
 .byte 0x9a
 .word 0x0e5e
diskseg: .word 0x6a5
 add sp,2
 call dump
 inc word ptr cs:[number]
 cmp word ptr cs:[number],2
 jb disk_loop
 mov bx,cs:[handle]
 mov ah,0x3e
 int 0x21
 mov ax,0x4c00
 int 0x21
fail:
 mov ax,0x4c01
 int 0x21
dump:
 pusha
 push ds
 mov si,ds:[0x9d4e]
 xor bp,bp
plane:
 mov dx,0x3ce
 mov ax,bp
 mov ah,al
 mov al,4
 out dx,ax
 mov ax,0xa000
 mov ds,ax
 mov dx,si
 mov cx,16000
 mov bx,cs:[handle]
 mov ah,0x40
 int 0x21
 jc fail
 cmp ax,16000
 jne fail
 inc bp
 cmp bp,4
 jb plane
 pop ds
 popa
 ret
filename: .asciz "PIXELS.BIN"
handle: .word 0
number: .word 0
