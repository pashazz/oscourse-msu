#include <inc/lib.h>
#include <inc/signal.h>

void
handler(int val)
{
    cprintf("receiving %d\n", val);
    int ret = 1;
    for (int i = 2; i <= val; ++i)
    {
        ret *= i;
    }
    cprintf("%d! = %d\n", val, ret);
    return;
}

void
umain(int argc, char **argv)
{
    struct sigaction sa = {handler};
    sys_sigaction(SIG_test, &sa);
    while (1) {}
}
