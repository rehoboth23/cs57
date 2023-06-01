	.file "semantic_tests/p1.c"
	.text
	.global func
	.type func, @function
func:
.b1:
	push r14
	mov r13, r14
	sub #8, r13	
	mov #10, -4(r14)
	mov 8(r14), r0
	mov #10, r2
	add r1, r2
	mov r2, r1
	cmp #100, r1
jl .b3
jmp .b2

.b2:
	mov #110, -8(r14)
	jmp .b4

.b3:
	mov #30, -8(r14)
	jmp .b4

.b4:
	mov #30, r0
	mov r14, r13
	pop r14
	ret

