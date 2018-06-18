#include <inc/lib.h>
#include <inc/signal.h>

void
handler(int val)
{
    cprintf("receiving %d\n", val);
    return;
}

void
umain(int argc, char **argv)
{
    struct sigaction sa = {handler};
    sys_sigaction(SIG_test, &sa);
    sys_sigqueue(sys_getenvid(), SIG_test, 1234);
    cprintf("handled\n");
    while (1) {}
}
