	.file	"test.c"
	.text
	.globl	get_firmware_version
	.type	get_firmware_version, @function
get_firmware_version:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movq	%rdx, -40(%rbp)
	movl	%ecx, %eax
	movb	%al, -44(%rbp)
	movb	$0, -1(%rbp)
	movabsq	$3473746674818101809, %rax
	movq	%rax, -10(%rbp)
	movb	$0, -2(%rbp)
	jmp	.L2
.L4:
	movq	-24(%rbp), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, -24(%rbp)
	movzbl	-1(%rbp), %edx
	leal	1(%rdx), %ecx
	movb	%cl, -1(%rbp)
	movzbl	%dl, %ecx
	movq	-40(%rbp), %rdx
	addq	%rcx, %rdx
	movzbl	(%rax), %eax
	movb	%al, (%rdx)
.L2:
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	testb	%al, %al
	je	.L3
	movzbl	-1(%rbp), %eax
	cmpb	-44(%rbp), %al
	jb	.L4
.L3:
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movl	$50, %edx
	movb	%dl, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movl	$48, %edx
	movb	%dl, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movl	$50, %edx
	movb	%dl, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movl	$51, %edx
	movb	%dl, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$46, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$48, (%rax)
	movl	$74, %eax
	cmpb	$74, %al
	jne	.L5
	movl	$97, %eax
	cmpb	$97, %al
	jne	.L5
	movl	$110, %eax
	cmpb	$110, %al
	jne	.L5
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$49, (%rax)
	jmp	.L6
.L5:
	movl	$74, %eax
	cmpb	$70, %al
	jne	.L7
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$50, (%rax)
	jmp	.L6
.L7:
	movl	$74, %eax
	cmpb	$77, %al
	jne	.L8
	movl	$97, %eax
	cmpb	$97, %al
	jne	.L8
	movl	$110, %eax
	cmpb	$114, %al
	jne	.L8
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$51, (%rax)
	jmp	.L6
.L8:
	movl	$74, %eax
	cmpb	$65, %al
	jne	.L9
	movl	$97, %eax
	cmpb	$112, %al
	jne	.L9
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$52, (%rax)
	jmp	.L6
.L9:
	movl	$74, %eax
	cmpb	$77, %al
	jne	.L10
	movl	$97, %eax
	cmpb	$97, %al
	jne	.L10
	movl	$110, %eax
	cmpb	$121, %al
	jne	.L10
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$53, (%rax)
	jmp	.L6
.L10:
	movl	$74, %eax
	cmpb	$74, %al
	jne	.L11
	movl	$97, %eax
	cmpb	$117, %al
	jne	.L11
	movl	$110, %eax
	cmpb	$110, %al
	jne	.L11
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$54, (%rax)
	jmp	.L6
.L11:
	movl	$74, %eax
	cmpb	$74, %al
	jne	.L12
	movl	$97, %eax
	cmpb	$117, %al
	jne	.L12
	movl	$110, %eax
	cmpb	$108, %al
	jne	.L12
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$55, (%rax)
	jmp	.L6
.L12:
	movl	$74, %eax
	cmpb	$65, %al
	jne	.L13
	movl	$97, %eax
	cmpb	$117, %al
	jne	.L13
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$56, (%rax)
	jmp	.L6
.L13:
	movl	$74, %eax
	cmpb	$83, %al
	jne	.L14
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$57, (%rax)
	jmp	.L6
.L14:
	movzbl	-1(%rbp), %eax
	subl	$1, %eax
	movb	%al, -1(%rbp)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$49, (%rax)
	movl	$74, %eax
	cmpb	$79, %al
	jne	.L15
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$48, (%rax)
	jmp	.L6
.L15:
	movl	$74, %eax
	cmpb	$78, %al
	jne	.L16
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$49, (%rax)
	jmp	.L6
.L16:
	movl	$74, %eax
	cmpb	$68, %al
	jne	.L6
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$50, (%rax)
.L6:
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$46, (%rax)
	movl	$50, %eax
	cmpb	$32, %al
	je	.L17
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movl	$50, %edx
	movb	%dl, (%rax)
.L17:
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movl	$50, %edx
	movb	%dl, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$95, (%rax)
	movzbl	-1(%rbp), %eax
	leal	1(%rax), %edx
	movb	%dl, -1(%rbp)
	movzbl	%al, %edx
	movq	-40(%rbp), %rax
	addq	%rdx, %rax
	movb	$0, (%rax)
	movzbl	-1(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	get_firmware_version, .-get_firmware_version
	.section	.rodata
.LC0:
	.string	"_rc3"
	.string	""
.LC1:
	.string	"0.4.1_b_"
	.string	""
.LC2:
	.string	"%s"
	.text
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	leaq	-32(%rbp), %rax
	movl	$30, %ecx
	movq	%rax, %rdx
	leaq	.LC0(%rip), %rsi
	leaq	.LC1(%rip), %rdi
	call	get_firmware_version
	leaq	-32(%rbp), %rax
	movq	%rax, %rsi
	leaq	.LC2(%rip), %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (Debian 10.2.1-6) 10.2.1 20210110"
	.section	.note.GNU-stack,"",@progbits
