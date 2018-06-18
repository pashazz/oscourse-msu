#include <inc/vsyscall.h>
#include <inc/lib.h>

static inline int32_t
vsyscall(int num)
{
	return vsys[num];
}

int vsys_gettime(void)
{
	return vsyscall(VSYS_gettime);
}

void
vsys_sigreturn()
{
	__asm __volatile("\tmovl 8(%ebp),%eax\n"
        "\ttestl %eax,%eax\n"
        "\tjz .finish\n"
        "\taddl $0x10,%esp\n"
        "\tret\n"
        "\t.finish:");
    sys_sigqueue(0, -1, 0);
    __asm __volatile("\taddl $0x14,%esp\n"
        "\tpopal\n"
		"\tpopl %es\n"
		"\tpopl %ds\n"
		"\taddl $0x8,%esp\n" /* skip tf_trapno and tf_errcode */
        "\taddl $0x8,%esp\n"
        "\tpopfl\n"
        "\taddl $0x8,%esp\n"
		"\tret");
}
