	.file	1 "gen_structsem.c"
	.section .mdebug.abi32
	.previous
	.gnu_attribute 4, 3
	.abicalls
	.text
	.align	2
	.globl	dummy
	.set	nomips16
	.ent	dummy
	.type	dummy, @function
dummy:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
#APP
 # 8 "libpthread/nptl/sysdeps/unix/sysv/linux/gen_structsem.c" 1
	@@@name@@@VALUE@@@value@@@0@@@end@@@
 # 0 "" 2
 # 9 "libpthread/nptl/sysdeps/unix/sysv/linux/gen_structsem.c" 1
	@@@name@@@PRIVATE@@@value@@@4@@@end@@@
 # 0 "" 2
 # 10 "libpthread/nptl/sysdeps/unix/sysv/linux/gen_structsem.c" 1
	@@@name@@@NWAITERS@@@value@@@8@@@end@@@
 # 0 "" 2
 # 11 "libpthread/nptl/sysdeps/unix/sysv/linux/gen_structsem.c" 1
	@@@name@@@SEM_VALUE_MAX@@@value@@@2147483647@@@end@@@
 # 0 "" 2
#NO_APP
	j	$31
	.end	dummy
	.size	dummy, .-dummy
	.ident	"GCC: (Buildroot 2012.11.1) 4.6.3"
