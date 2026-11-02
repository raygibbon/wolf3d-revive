/* SPDX-License-Identifier: GPL-2.0-only */
/* Private DOS reference logger. Addresses checked against executable SHA-256. */
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
 push cs
 pop es
 mov word ptr es:[number],0
next_demo:
 push word ptr es:[number]
 # call original PlayDemo, relocated segment word by Python
 .byte 0x9a
 .word 0xdd1
playseg: .word 0xbc6
 add sp,2
 push cs
 pop es
 inc word ptr es:[number]
 cmp word ptr es:[number],4
 jb next_demo
 mov ax,0x4c00
 int 0x21
# called instead of UpdateSoundLoc
frame:
 .byte 0x9a
 .word 0x148
soundseg: .word 0xbc6
 pushf
 pusha
 push ds
 push es
 cmp word ptr ds:[0xa2b6],0
 je done
 cmp word ptr cs:[handle],0
 jne opened
 push ds
 push cs
 pop ds
 mov dx,offset filename
 xor cx,cx
 mov ah,0x3c
 int 0x21
 pop ds
 jc fail
 mov cs:[handle],ax
opened:
 mov bx,cs:[handle]
 # metadata: gamestate, object list, player pointer, commands, RNG
 mov dx,0x95c2
 mov cx,66
 call write
 mov dx,0x95bc
 mov cx,2
 call write
 mov dx,0xa2b2
 mov cx,4
 call write
 mov dx,0xa4ea
 mov cx,20
 call write
 mov dx,0xb51c
 mov cx,9000
 call write
 push ds
 mov ax,ds
 sub ax,0x172b
 mov ds,ax
 xor dx,dx
 mov cx,2
 call write
 pop ds
 push ds
 les si,ds:[0xa2b2]
 sub si,3
 mov dx,si
 push es
 pop ds
 mov cx,3
 call write
 pop ds
 mov dx,0xa516
 mov cx,4096
 call write
 mov dx,0x75ba
 mov cx,4096
 call write
 mov dx,0x5328
 mov cx,128
 call write
 mov dx,0x86ba
 mov cx,640
 call write
 mov dx,0x55ba
 mov cx,8192
 call write
 mov dx,0xd85c
 mov cx,2
 call write
 jmp done
fail:
 mov ax,0x4c01
 int 0x21
done:
 pop es
 pop ds
 popa
 popf
 retf
write:
 mov ah,0x40
 int 0x21
 jc fail
 cmp ax,cx
 jne fail
 ret
number: .word 0
handle: .word 0
filename: .asciz "TRACE.BIN"
