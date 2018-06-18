// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <inc/lib.h>
#include <inc/signal.h>

int i;

void
handler(int val)
{
    i = val;
    cprintf("%x got %d\n", sys_getenvid(), val);
    if (val >= 10) exit();
}

void
umain(int argc, char **argv)
{
    struct sigaction sa = {handler};
    sys_sigaction(SIG_test, &sa);

    envid_t parent = sys_getenvid();
	envid_t who;

	if ((who = fork()) == 0) {
		// get the ball rolling
        who = parent;
		cprintf("send 0 from %x to %x\n", sys_getenvid(), who);
		sys_sigqueue(who, SIG_test, 0);
	}

	while (1) {
        sys_sigwait(0);
        sys_sigqueue(who, SIG_test, i + 1);
	}

}

