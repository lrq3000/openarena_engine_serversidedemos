	.file	"g_arenas.c"
	.text
	.p2align 4,,15
	.type	CelebrateStop, @function
CelebrateStop:
.LFB70:
	xorl	%edx, %edx
	cmpl	$1, 192(%rdi)
	movl	200(%rdi), %eax
	notl	%eax
	sete	%dl
	andl	$128, %eax
	addl	$11, %edx
	orl	%eax, %edx
	movl	%edx, 200(%rdi)
	ret
.LFE70:
	.size	CelebrateStop, .-CelebrateStop
	.p2align 4,,15
.globl Svcmd_AbortPodium_f
	.type	Svcmd_AbortPodium_f, @function
Svcmd_AbortPodium_f:
.LFB75:
	cmpl	$2, g_gametype+12(%rip)
	je	.L9
.L7:
	rep
	ret
	.p2align 4,,10
	.p2align 3
.L9:
	movq	podium1(%rip), %rdx
	testq	%rdx, %rdx
	je	.L7
	movl	level+40(%rip), %eax
	movq	$CelebrateStop, 768(%rdx)
	movl	%eax, 760(%rdx)
	ret
.LFE75:
	.size	Svcmd_AbortPodium_f, .-Svcmd_AbortPodium_f
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"g_podiumDist"
.LC1:
	.string	"g_podiumDrop"
	.text
	.p2align 4,,15
	.type	PodiumPlacementThink, @function
PodiumPlacementThink:
.LFB72:
	pushq	%rbp
.LCFI0:
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	pushq	%rbx
.LCFI1:
	movq	%rdi, %rbx
	subq	$120, %rsp
.LCFI2:
	movl	level+40(%rip), %eax
	leaq	96(%rsp), %rbp
	addl	$100, %eax
	movq	%rbp, %rsi
	movl	%eax, 760(%rdi)
	movl	$level+9684, %edi
	call	AngleVectors
	movss	level+9672(%rip), %xmm0
	movl	$.LC0, %edi
	movss	96(%rsp), %xmm1
	movss	%xmm0, 8(%rsp)
	movss	%xmm1, 12(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	movl	$.LC0, %edi
	movss	8(%rsp), %xmm1
	mulss	12(%rsp), %xmm0
	addss	%xmm0, %xmm1
	movss	level+9676(%rip), %xmm0
	movss	%xmm0, 16(%rsp)
	movss	%xmm1, 80(%rsp)
	movss	100(%rsp), %xmm1
	movss	%xmm1, 20(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	movl	$.LC0, %edi
	movss	16(%rsp), %xmm1
	mulss	20(%rsp), %xmm0
	addss	%xmm0, %xmm1
	movss	level+9680(%rip), %xmm0
	movss	%xmm0, 28(%rsp)
	movss	%xmm1, 84(%rsp)
	movss	104(%rsp), %xmm1
	movss	%xmm1, 24(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	movl	$.LC1, %edi
	movss	28(%rsp), %xmm1
	mulss	24(%rsp), %xmm0
	addss	%xmm0, %xmm1
	movss	%xmm1, 28(%rsp)
	movss	%xmm1, 88(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	leaq	80(%rsp), %rsi
	movq	%rbx, %rdi
	movss	28(%rsp), %xmm1
	subss	%xmm0, %xmm1
	movss	%xmm1, 88(%rsp)
	call	G_SetOrigin
	movq	podium1(%rip), %rax
	testq	%rax, %rax
	je	.L11
	movss	level+9672(%rip), %xmm0
	leaq	60(%rax), %rsi
	subss	488(%rbx), %xmm0
	movq	%rbp, %rdi
	movss	%xmm0, 96(%rsp)
	movss	level+9676(%rip), %xmm0
	subss	492(%rbx), %xmm0
	movss	%xmm0, 100(%rsp)
	movss	level+9680(%rip), %xmm0
	subss	496(%rbx), %xmm0
	movss	%xmm0, 104(%rsp)
	call	vectoangles
	movq	podium1(%rip), %rdi
	xorl	%eax, %eax
	leaq	64(%rsp), %rsi
	leaq	32(%rsp), %rcx
	leaq	48(%rsp), %rdx
	movl	%eax, 60(%rdi)
	movl	%eax, 68(%rdi)
	addq	$60, %rdi
	call	AngleVectors
	movss	offsetFirst(%rip), %xmm0
	movq	podium1(%rip), %rdi
	movaps	%xmm0, %xmm3
	movq	%rbp, %rsi
	movss	offsetFirst+4(%rip), %xmm5
	mulss	68(%rsp), %xmm3
	movaps	%xmm5, %xmm1
	movaps	%xmm0, %xmm2
	mulss	64(%rsp), %xmm0
	mulss	52(%rsp), %xmm1
	movss	offsetFirst+8(%rip), %xmm4
	mulss	72(%rsp), %xmm2
	addss	492(%rbx), %xmm3
	addss	488(%rbx), %xmm0
	addss	496(%rbx), %xmm2
	addss	%xmm1, %xmm3
	movaps	%xmm5, %xmm1
	mulss	48(%rsp), %xmm5
	mulss	56(%rsp), %xmm1
	addss	%xmm5, %xmm0
	addss	%xmm1, %xmm2
	movaps	%xmm4, %xmm1
	mulss	32(%rsp), %xmm1
	addss	%xmm1, %xmm0
	movss	%xmm0, 96(%rsp)
	movaps	%xmm4, %xmm0
	mulss	40(%rsp), %xmm4
	mulss	36(%rsp), %xmm0
	addss	%xmm4, %xmm2
	addss	%xmm0, %xmm3
	movss	%xmm2, 104(%rsp)
	movss	%xmm3, 100(%rsp)
	call	G_SetOrigin
.L11:
	movq	podium2(%rip), %rax
	testq	%rax, %rax
	je	.L12
	movss	level+9672(%rip), %xmm0
	leaq	60(%rax), %rsi
	subss	488(%rbx), %xmm0
	movq	%rbp, %rdi
	movss	%xmm0, 96(%rsp)
	movss	level+9676(%rip), %xmm0
	subss	492(%rbx), %xmm0
	movss	%xmm0, 100(%rsp)
	movss	level+9680(%rip), %xmm0
	subss	496(%rbx), %xmm0
	movss	%xmm0, 104(%rsp)
	call	vectoangles
	movq	podium2(%rip), %rdi
	xorl	%eax, %eax
	leaq	64(%rsp), %rsi
	leaq	32(%rsp), %rcx
	leaq	48(%rsp), %rdx
	movl	%eax, 60(%rdi)
	movl	%eax, 68(%rdi)
	addq	$60, %rdi
	call	AngleVectors
	movss	offsetSecond(%rip), %xmm0
	movq	podium2(%rip), %rdi
	movaps	%xmm0, %xmm3
	movq	%rbp, %rsi
	movss	offsetSecond+4(%rip), %xmm5
	mulss	68(%rsp), %xmm3
	movaps	%xmm5, %xmm1
	movaps	%xmm0, %xmm2
	mulss	64(%rsp), %xmm0
	mulss	52(%rsp), %xmm1
	movss	offsetSecond+8(%rip), %xmm4
	mulss	72(%rsp), %xmm2
	addss	492(%rbx), %xmm3
	addss	488(%rbx), %xmm0
	addss	496(%rbx), %xmm2
	addss	%xmm1, %xmm3
	movaps	%xmm5, %xmm1
	mulss	48(%rsp), %xmm5
	mulss	56(%rsp), %xmm1
	addss	%xmm5, %xmm0
	addss	%xmm1, %xmm2
	movaps	%xmm4, %xmm1
	mulss	32(%rsp), %xmm1
	addss	%xmm1, %xmm0
	movss	%xmm0, 96(%rsp)
	movaps	%xmm4, %xmm0
	mulss	40(%rsp), %xmm4
	mulss	36(%rsp), %xmm0
	addss	%xmm4, %xmm2
	addss	%xmm0, %xmm3
	movss	%xmm2, 104(%rsp)
	movss	%xmm3, 100(%rsp)
	call	G_SetOrigin
.L12:
	movq	podium3(%rip), %rax
	testq	%rax, %rax
	je	.L14
	movss	level+9672(%rip), %xmm0
	leaq	60(%rax), %rsi
	subss	488(%rbx), %xmm0
	movq	%rbp, %rdi
	movss	%xmm0, 96(%rsp)
	movss	level+9676(%rip), %xmm0
	subss	492(%rbx), %xmm0
	movss	%xmm0, 100(%rsp)
	movss	level+9680(%rip), %xmm0
	subss	496(%rbx), %xmm0
	movss	%xmm0, 104(%rsp)
	call	vectoangles
	movq	podium3(%rip), %rdi
	xorl	%eax, %eax
	leaq	64(%rsp), %rsi
	leaq	32(%rsp), %rcx
	leaq	48(%rsp), %rdx
	movl	%eax, 60(%rdi)
	movl	%eax, 68(%rdi)
	addq	$60, %rdi
	call	AngleVectors
	movss	offsetThird(%rip), %xmm0
	movq	podium3(%rip), %rdi
	movaps	%xmm0, %xmm3
	movq	%rbp, %rsi
	movss	offsetThird+4(%rip), %xmm5
	mulss	68(%rsp), %xmm3
	movaps	%xmm5, %xmm1
	movaps	%xmm0, %xmm2
	mulss	64(%rsp), %xmm0
	mulss	52(%rsp), %xmm1
	movss	offsetThird+8(%rip), %xmm4
	mulss	72(%rsp), %xmm2
	addss	492(%rbx), %xmm3
	addss	488(%rbx), %xmm0
	addss	496(%rbx), %xmm2
	addss	%xmm1, %xmm3
	movaps	%xmm5, %xmm1
	mulss	48(%rsp), %xmm5
	mulss	56(%rsp), %xmm1
	addss	%xmm5, %xmm0
	addss	%xmm1, %xmm2
	movaps	%xmm4, %xmm1
	mulss	32(%rsp), %xmm1
	addss	%xmm1, %xmm0
	movss	%xmm0, 96(%rsp)
	movaps	%xmm4, %xmm0
	mulss	40(%rsp), %xmm4
	mulss	36(%rsp), %xmm0
	addss	%xmm4, %xmm2
	addss	%xmm0, %xmm3
	movss	%xmm2, 104(%rsp)
	movss	%xmm3, 100(%rsp)
	call	G_SetOrigin
.L14:
	addq	$120, %rsp
	popq	%rbx
	popq	%rbp
	ret
.LFE72:
	.size	PodiumPlacementThink, .-PodiumPlacementThink
	.section	.rodata.str1.1
.LC3:
	.string	"^1ERROR: out of gentities\n"
	.text
	.p2align 4,,15
	.type	SpawnModelOnVictoryPad, @function
SpawnModelOnVictoryPad:
.LFB69:
	pushq	%r15
.LCFI3:
	movl	%ecx, %r15d
	pushq	%r14
.LCFI4:
	movq	%rdi, %r14
	pushq	%r13
.LCFI5:
	movq	%rsi, %r13
	pushq	%r12
.LCFI6:
	pushq	%rbp
.LCFI7:
	pushq	%rbx
.LCFI8:
	movq	%rdx, %rbx
	subq	$72, %rsp
.LCFI9:
	call	G_Spawn
	testq	%rax, %rax
	movq	%rax, %rbp
	je	.L21
	movq	520(%rbx), %rdx
	movl	$26, %ecx
	movq	%rbp, %rdi
	movq	%rbx, %rsi
	movl	$1, 592(%rbp)
	movl	$0x00000000, 596(%rbp)
	rep movsq
	leaq	512(%rdx), %rax
	movq	%rdx, 520(%rbp)
	movabsq	$-3751880150584993549, %rdx
	movl	$1, 4(%rbp)
	movl	$0, 8(%rbp)
	movq	%rax, 536(%rbp)
	movq	%rbp, %rax
	movl	$0, 188(%rbp)
	subq	$g_entities, %rax
	movl	$0, 156(%rbp)
	movl	$0, 12(%rbp)
	sarq	$4, %rax
	imulq	%rdx, %rax
	movl	%eax, (%rbp)
	movl	level+40(%rip), %eax
	movl	$0, 180(%rbp)
	movl	$1022, 148(%rbp)
	movl	$22, 196(%rbp)
	movl	$11, 200(%rbp)
	movl	%eax, 688(%rbp)
	movl	192(%rbp), %eax
	testl	%eax, %eax
	je	.L22
	subl	$1, %eax
	jne	.L19
	movl	$12, 200(%rbp)
.L19:
	movl	424(%rbx), %eax
	movl	$0, 180(%rbp)
	leaq	48(%rsp), %r12
	movl	$65537, 600(%rbp)
	movl	$33554432, 460(%rbp)
	movl	$0, 840(%rbp)
	movq	%r12, %rdi
	movl	%eax, 424(%rbp)
	movl	436(%rbx), %eax
	movl	%eax, 436(%rbp)
	movl	440(%rbx), %eax
	movl	%eax, 440(%rbp)
	movl	444(%rbx), %eax
	movl	%eax, 444(%rbp)
	movl	448(%rbx), %eax
	movl	%eax, 448(%rbp)
	movl	452(%rbx), %eax
	movl	%eax, 452(%rbp)
	movl	456(%rbx), %eax
	movl	%eax, 456(%rbp)
	movl	464(%rbx), %eax
	movl	%eax, 464(%rbp)
	movl	468(%rbx), %eax
	movl	%eax, 468(%rbp)
	movl	472(%rbx), %eax
	movl	%eax, 472(%rbp)
	movl	476(%rbx), %eax
	movl	%eax, 476(%rbp)
	movl	480(%rbx), %eax
	movl	%eax, 480(%rbp)
	movl	484(%rbx), %eax
	movl	%eax, 484(%rbp)
	movl	512(%rbx), %eax
	leaq	60(%rbp), %rbx
	movss	level+9672(%rip), %xmm0
	subss	488(%r14), %xmm0
	movq	%rbx, %rsi
	movl	%eax, 512(%rbp)
	movss	%xmm0, 48(%rsp)
	movss	level+9676(%rip), %xmm0
	subss	492(%r14), %xmm0
	movss	%xmm0, 52(%rsp)
	movss	level+9680(%rip), %xmm0
	subss	496(%r14), %xmm0
	movss	%xmm0, 56(%rsp)
	call	vectoangles
	leaq	16(%rsp), %rdx
	leaq	32(%rsp), %rsi
	movq	%rsp, %rcx
	movq	%rbx, %rdi
	movl	$0x00000000, 60(%rbp)
	movl	$0x00000000, 68(%rbp)
	call	AngleVectors
	movss	32(%rsp), %xmm1
	movq	%r12, %rsi
	mulss	(%r13), %xmm1
	movss	16(%rsp), %xmm0
	mulss	4(%r13), %xmm0
	movss	36(%rsp), %xmm2
	mulss	(%r13), %xmm2
	movss	40(%rsp), %xmm3
	mulss	(%r13), %xmm3
	movq	%rbp, %rdi
	addss	488(%r14), %xmm1
	addss	492(%r14), %xmm2
	addss	496(%r14), %xmm3
	addss	%xmm0, %xmm1
	movss	20(%rsp), %xmm0
	mulss	4(%r13), %xmm0
	addss	%xmm0, %xmm2
	movss	24(%rsp), %xmm0
	mulss	4(%r13), %xmm0
	addss	%xmm0, %xmm3
	movss	(%rsp), %xmm0
	mulss	8(%r13), %xmm0
	addss	%xmm0, %xmm1
	movss	4(%rsp), %xmm0
	mulss	8(%r13), %xmm0
	movss	%xmm1, 48(%rsp)
	addss	%xmm0, %xmm2
	movss	8(%rsp), %xmm0
	mulss	8(%r13), %xmm0
	movss	%xmm2, 52(%rsp)
	addss	%xmm0, %xmm3
	movss	%xmm3, 56(%rsp)
	call	G_SetOrigin
	movq	%rbp, %rdi
	call	trap_LinkEntity
	movl	%r15d, 864(%rbp)
.L17:
	addq	$72, %rsp
	movq	%rbp, %rax
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
	.p2align 4,,10
	.p2align 3
.L22:
	movl	$2, 192(%rbp)
	jmp	.L19
.L21:
	movl	$.LC3, %edi
	xorl	%eax, %eax
	call	G_Printf
	jmp	.L17
.LFE69:
	.size	SpawnModelOnVictoryPad, .-SpawnModelOnVictoryPad
	.section	.rodata.str1.1
.LC4:
	.string	"podium"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC5:
	.string	"models/mapobjects/podium/podium4.md3"
	.text
	.p2align 4,,15
.globl SpawnModelsOnVictoryPads
	.type	SpawnModelsOnVictoryPads, @function
SpawnModelsOnVictoryPads:
.LFB74:
	pushq	%rbp
.LCFI10:
	pushq	%rbx
.LCFI11:
	subq	$72, %rsp
.LCFI12:
	movq	$0, podium1(%rip)
	movq	$0, podium2(%rip)
	movq	$0, podium3(%rip)
	call	G_Spawn
	testq	%rax, %rax
	movq	%rax, %rbp
	je	.L24
	movq	$.LC4, 536(%rax)
	movl	$0, 4(%rax)
	subq	$g_entities, %rax
	movabsq	$-3751880150584993549, %rdx
	sarq	$4, %rax
	leaq	48(%rsp), %rbx
	imulq	%rdx, %rax
	movl	$.LC5, %edi
	movl	$1, 600(%rbp)
	movl	$1, 460(%rbp)
	movl	%eax, (%rbp)
	call	G_ModelIndex
	xorl	%ecx, %ecx
	xorl	%edx, %edx
	movq	%rbx, %rsi
	movl	%eax, 160(%rbp)
	movl	$level+9684, %edi
	call	AngleVectors
	movss	level+9672(%rip), %xmm0
	movl	$.LC0, %edi
	movss	48(%rsp), %xmm1
	movss	%xmm0, 28(%rsp)
	movss	%xmm1, 24(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	movl	$.LC0, %edi
	movss	28(%rsp), %xmm1
	mulss	24(%rsp), %xmm0
	addss	%xmm0, %xmm1
	movss	level+9676(%rip), %xmm0
	movss	%xmm0, 20(%rsp)
	movss	%xmm1, 32(%rsp)
	movss	52(%rsp), %xmm1
	movss	%xmm1, 16(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	movl	$.LC0, %edi
	movss	20(%rsp), %xmm1
	mulss	16(%rsp), %xmm0
	addss	%xmm0, %xmm1
	movss	level+9680(%rip), %xmm0
	movss	%xmm0, 8(%rsp)
	movss	%xmm1, 36(%rsp)
	movss	56(%rsp), %xmm1
	movss	%xmm1, 12(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	movl	$.LC1, %edi
	movss	8(%rsp), %xmm1
	mulss	12(%rsp), %xmm0
	addss	%xmm0, %xmm1
	movss	%xmm1, 8(%rsp)
	movss	%xmm1, 40(%rsp)
	call	trap_Cvar_VariableIntegerValue
	cvtsi2ss	%eax, %xmm0
	leaq	32(%rsp), %rsi
	movq	%rbp, %rdi
	movss	8(%rsp), %xmm1
	subss	%xmm0, %xmm1
	movss	%xmm1, 40(%rsp)
	call	G_SetOrigin
	movss	level+9672(%rip), %xmm0
	movq	%rbx, %rdi
	subss	488(%rbp), %xmm0
	movss	%xmm0, 48(%rsp)
	movss	level+9676(%rip), %xmm0
	subss	492(%rbp), %xmm0
	movss	%xmm0, 52(%rsp)
	movss	level+9680(%rip), %xmm0
	subss	496(%rbp), %xmm0
	movss	%xmm0, 56(%rsp)
	call	vectoyaw
	movss	%xmm0, 64(%rbp)
	movq	%rbp, %rdi
	call	trap_LinkEntity
	movl	level+40(%rip), %eax
	movq	$PodiumPlacementThink, 768(%rbp)
	addl	$100, %eax
	movl	%eax, 760(%rbp)
.L24:
	movslq	level+92(%rip),%rdx
	movq	level(%rip), %rax
	movl	$offsetFirst, %esi
	movq	%rbp, %rdi
	imulq	$784, %rdx, %rcx
	imulq	$944, %rdx, %rdx
	movl	256(%rcx,%rax), %ecx
	addq	$g_entities, %rdx
	andb	$191, %ch
	call	SpawnModelOnVictoryPad
	testq	%rax, %rax
	movq	%rax, %rdx
	je	.L25
	movl	level+40(%rip), %eax
	movq	$CelebrateStart, 768(%rdx)
	movq	%rdx, podium1(%rip)
	addl	$2000, %eax
	movl	%eax, 760(%rdx)
.L25:
	movslq	level+96(%rip),%rdx
	movq	level(%rip), %rax
	movl	$offsetSecond, %esi
	movq	%rbp, %rdi
	imulq	$784, %rdx, %rcx
	imulq	$944, %rdx, %rdx
	movl	256(%rcx,%rax), %ecx
	addq	$g_entities, %rdx
	andb	$191, %ch
	call	SpawnModelOnVictoryPad
	testq	%rax, %rax
	je	.L26
	movq	%rax, podium2(%rip)
.L26:
	cmpl	$2, level+84(%rip)
	jg	.L29
.L28:
	addq	$72, %rsp
	popq	%rbx
	popq	%rbp
	ret
	.p2align 4,,10
	.p2align 3
.L29:
	movslq	level+100(%rip),%rdx
	movq	level(%rip), %rax
	movl	$offsetThird, %esi
	movq	%rbp, %rdi
	imulq	$784, %rdx, %rcx
	imulq	$944, %rdx, %rdx
	movl	256(%rcx,%rax), %ecx
	addq	$g_entities, %rdx
	andb	$191, %ch
	call	SpawnModelOnVictoryPad
	testq	%rax, %rax
	je	.L28
	movq	%rax, podium3(%rip)
	addq	$72, %rsp
	popq	%rbx
	popq	%rbp
	ret
.LFE74:
	.size	SpawnModelsOnVictoryPads, .-SpawnModelsOnVictoryPads
	.p2align 4,,15
	.type	CelebrateStart, @function
CelebrateStart:
.LFB71:
	movl	200(%rdi), %eax
	movq	$CelebrateStop, 768(%rdi)
	xorl	%edx, %edx
	movl	$76, %esi
	notl	%eax
	andl	$128, %eax
	orl	$6, %eax
	movl	%eax, 200(%rdi)
	movl	level+40(%rip), %eax
	addl	$2294, %eax
	movl	%eax, 760(%rdi)
	jmp	G_AddEvent
.LFE71:
	.size	CelebrateStart, .-CelebrateStart
	.section	.rodata.str1.1
.LC6:
	.string	"postgame %i %i 0 0 0 0 0 0"
	.section	.rodata.str1.8
	.align 8
.LC7:
	.string	"postgame %i %i %i %i %i %i %i %i"
	.section	.rodata.str1.1
.LC8:
	.string	" %i %i %i"
	.text
	.p2align 4,,15
.globl UpdateTournamentInfo
	.type	UpdateTournamentInfo, @function
UpdateTournamentInfo:
.LFB68:
	pushq	%r14
.LCFI13:
	pushq	%r13
.LCFI14:
	pushq	%r12
.LCFI15:
	pushq	%rbp
.LCFI16:
	pushq	%rbx
.LCFI17:
	subq	$1120, %rsp
.LCFI18:
	movl	level+32(%rip), %edx
	movq	%fs:40, %rax
	movq	%rax, 1112(%rsp)
	xorl	%eax, %eax
	testl	%edx, %edx
	jle	.L49
	movl	$g_entities, %eax
	xorl	%ebx, %ebx
	.p2align 4,,10
	.p2align 3
.L36:
	movl	528(%rax), %r8d
	movq	%rax, %rbp
	testl	%r8d, %r8d
	je	.L34
	testb	$8, 424(%rax)
	je	.L35
.L34:
	addl	$1, %ebx
	addq	$944, %rax
	cmpl	%edx, %ebx
	jl	.L36
.L35:
	testq	%rbp, %rbp
	je	.L49
	cmpl	%edx, %ebx
	je	.L49
	.p2align 4,,7
	.p2align 3
	call	CalculateRanks
	movslq	%ebx,%rax
	imulq	$784, %rax, %r8
	addq	level(%rip), %r8
	cmpl	$3, 616(%r8)
	je	.L55
	movq	520(%rbp), %rsi
	xorl	%edi, %edi
	movl	712(%rsi), %ecx
	testl	%ecx, %ecx
	jne	.L56
	movl	256(%r8), %ecx
	xorl	%edx, %edx
	testl	%ecx, %ecx
	je	.L57
.L42:
	movl	%edx, 32(%rsp)
	movl	248(%rsi), %eax
	leaq	48(%rsp), %r13
	movl	level+84(%rip), %ecx
	movl	%edi, %r9d
	movl	%ebx, %r8d
	movl	$.LC7, %edx
	movq	%r13, %rdi
	movl	%eax, 24(%rsp)
	movl	300(%rsi), %eax
	movl	%eax, 16(%rsp)
	movl	288(%rsi), %eax
	movl	%eax, 8(%rsp)
	movl	284(%rsi), %eax
	movl	$1024, %esi
	movl	%eax, (%rsp)
	xorl	%eax, %eax
	call	Com_sprintf
.L38:
	movq	%r13, %rcx
.L43:
	movl	(%rcx), %eax
	addq	$4, %rcx
	leal	-16843009(%rax), %edx
	notl	%eax
	andl	%eax, %edx
	andl	$-2139062144, %edx
	je	.L43
	movl	%edx, %eax
	shrl	$16, %eax
	testl	$32896, %edx
	cmove	%eax, %edx
	leaq	2(%rcx), %rax
	cmove	%rax, %rcx
	addb	%dl, %dl
	movl	level+84(%rip), %eax
	sbbq	$3, %rcx
	movl	%ecx, %r14d
	subl	%r13d, %r14d
	testl	%eax, %eax
	jle	.L45
	leaq	1072(%rsp), %r12
	movl	$level+92, %ebp
	xorl	%ebx, %ebx
	.p2align 4,,10
	.p2align 3
.L48:
	movl	(%rbp), %ecx
	movl	$.LC8, %edx
	movl	$32, %esi
	movq	%r12, %rdi
	movslq	%ecx,%rax
	imulq	$784, %rax, %rax
	addq	level(%rip), %rax
	movl	248(%rax), %r9d
	movl	256(%rax), %r8d
	xorl	%eax, %eax
	call	Com_sprintf
	movq	%r12, %rcx
.L46:
	movl	(%rcx), %eax
	addq	$4, %rcx
	leal	-16843009(%rax), %edx
	notl	%eax
	andl	%eax, %edx
	andl	$-2139062144, %edx
	je	.L46
	movl	%edx, %eax
	shrl	$16, %eax
	testl	$32896, %edx
	cmove	%eax, %edx
	leaq	2(%rcx), %rax
	cmove	%rax, %rcx
	addb	%dl, %dl
	sbbq	$3, %rcx
	subq	%r12, %rcx
	leal	1(%r14,%rcx), %eax
	cmpl	$1023, %eax
	ja	.L45
	movl	$1024, %edx
	movq	%r12, %rsi
	movq	%r13, %rdi
	call	__strcat_chk
	addl	$1, %ebx
	addq	$4, %rbp
	cmpl	%ebx, level+84(%rip)
	jg	.L48
.L45:
	movq	%r13, %rsi
	movl	$2, %edi
	call	trap_SendConsoleCommand
.L49:
	movq	1112(%rsp), %rax
	xorq	%fs:40, %rax
	jne	.L58
	addq	$1120, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	ret
	.p2align 4,,10
	.p2align 3
.L57:
	xorl	%edx, %edx
	cmpl	$0, 280(%rsi)
	sete	%dl
	jmp	.L42
	.p2align 4,,10
	.p2align 3
.L56:
	movl	716(%rsi), %edx
	movl	$100, %edi
	imull	%edi, %edx
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	%ecx
	movl	256(%r8), %ecx
	xorl	%edx, %edx
	testl	%ecx, %ecx
	movl	%eax, %edi
	jne	.L42
	jmp	.L57
.L55:
	leaq	48(%rsp), %r13
	movl	level+84(%rip), %ecx
	movl	%ebx, %r8d
	movl	$.LC6, %edx
	movl	$1024, %esi
	xorl	%eax, %eax
	movq	%r13, %rdi
	call	Com_sprintf
	jmp	.L38
.L58:
	call	__stack_chk_fail
.LFE68:
	.size	UpdateTournamentInfo, .-UpdateTournamentInfo
	.data
	.align 4
	.type	offsetFirst, @object
	.size	offsetFirst, 12
offsetFirst:
	.long	0
	.long	0
	.long	1116995584
	.align 4
	.type	offsetSecond, @object
	.size	offsetSecond, 12
offsetSecond:
	.long	3240099840
	.long	1114636288
	.long	1113063424
	.align 4
	.type	offsetThird, @object
	.size	offsetThird, 12
offsetThird:
	.long	3247964160
	.long	3262119936
	.long	1110704128
	.comm	podium1,8,8
	.comm	podium2,8,8
	.comm	podium3,8,8
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1
.LSCIE1:
	.long	0x0
	.byte	0x1
	.string	"zR"
	.uleb128 0x1
	.sleb128 -8
	.byte	0x10
	.uleb128 0x1
	.byte	0x3
	.byte	0xc
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90
	.uleb128 0x1
	.align 8
.LECIE1:
.LSFDE1:
	.long	.LEFDE1-.LASFDE1
.LASFDE1:
	.long	.LASFDE1-.Lframe1
	.long	.LFB70
	.long	.LFE70-.LFB70
	.uleb128 0x0
	.align 8
.LEFDE1:
.LSFDE3:
	.long	.LEFDE3-.LASFDE3
.LASFDE3:
	.long	.LASFDE3-.Lframe1
	.long	.LFB75
	.long	.LFE75-.LFB75
	.uleb128 0x0
	.align 8
.LEFDE3:
.LSFDE5:
	.long	.LEFDE5-.LASFDE5
.LASFDE5:
	.long	.LASFDE5-.Lframe1
	.long	.LFB72
	.long	.LFE72-.LFB72
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI0-.LFB72
	.byte	0xe
	.uleb128 0x10
	.byte	0x4
	.long	.LCFI1-.LCFI0
	.byte	0xe
	.uleb128 0x18
	.byte	0x83
	.uleb128 0x3
	.byte	0x86
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI2-.LCFI1
	.byte	0xe
	.uleb128 0x90
	.align 8
.LEFDE5:
.LSFDE7:
	.long	.LEFDE7-.LASFDE7
.LASFDE7:
	.long	.LASFDE7-.Lframe1
	.long	.LFB69
	.long	.LFE69-.LFB69
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI3-.LFB69
	.byte	0xe
	.uleb128 0x10
	.byte	0x8f
	.uleb128 0x2
	.byte	0x4
	.long	.LCFI4-.LCFI3
	.byte	0xe
	.uleb128 0x18
	.byte	0x8e
	.uleb128 0x3
	.byte	0x4
	.long	.LCFI5-.LCFI4
	.byte	0xe
	.uleb128 0x20
	.byte	0x8d
	.uleb128 0x4
	.byte	0x4
	.long	.LCFI6-.LCFI5
	.byte	0xe
	.uleb128 0x28
	.byte	0x4
	.long	.LCFI7-.LCFI6
	.byte	0xe
	.uleb128 0x30
	.byte	0x4
	.long	.LCFI8-.LCFI7
	.byte	0xe
	.uleb128 0x38
	.byte	0x83
	.uleb128 0x7
	.byte	0x86
	.uleb128 0x6
	.byte	0x8c
	.uleb128 0x5
	.byte	0x4
	.long	.LCFI9-.LCFI8
	.byte	0xe
	.uleb128 0x80
	.align 8
.LEFDE7:
.LSFDE9:
	.long	.LEFDE9-.LASFDE9
.LASFDE9:
	.long	.LASFDE9-.Lframe1
	.long	.LFB74
	.long	.LFE74-.LFB74
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI10-.LFB74
	.byte	0xe
	.uleb128 0x10
	.byte	0x4
	.long	.LCFI11-.LCFI10
	.byte	0xe
	.uleb128 0x18
	.byte	0x4
	.long	.LCFI12-.LCFI11
	.byte	0xe
	.uleb128 0x60
	.byte	0x83
	.uleb128 0x3
	.byte	0x86
	.uleb128 0x2
	.align 8
.LEFDE9:
.LSFDE11:
	.long	.LEFDE11-.LASFDE11
.LASFDE11:
	.long	.LASFDE11-.Lframe1
	.long	.LFB71
	.long	.LFE71-.LFB71
	.uleb128 0x0
	.align 8
.LEFDE11:
.LSFDE13:
	.long	.LEFDE13-.LASFDE13
.LASFDE13:
	.long	.LASFDE13-.Lframe1
	.long	.LFB68
	.long	.LFE68-.LFB68
	.uleb128 0x0
	.byte	0x4
	.long	.LCFI13-.LFB68
	.byte	0xe
	.uleb128 0x10
	.byte	0x4
	.long	.LCFI14-.LCFI13
	.byte	0xe
	.uleb128 0x18
	.byte	0x4
	.long	.LCFI15-.LCFI14
	.byte	0xe
	.uleb128 0x20
	.byte	0x4
	.long	.LCFI16-.LCFI15
	.byte	0xe
	.uleb128 0x28
	.byte	0x4
	.long	.LCFI17-.LCFI16
	.byte	0xe
	.uleb128 0x30
	.byte	0x4
	.long	.LCFI18-.LCFI17
	.byte	0xe
	.uleb128 0x490
	.byte	0x83
	.uleb128 0x6
	.byte	0x86
	.uleb128 0x5
	.byte	0x8c
	.uleb128 0x4
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8e
	.uleb128 0x2
	.align 8
.LEFDE13:
	.ident	"GCC: (Ubuntu 4.3.3-5ubuntu4) 4.3.3"
	.section	.note.GNU-stack,"",@progbits
