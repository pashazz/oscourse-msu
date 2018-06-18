// Fork a binary tree of processes and display their structure.

#include <inc/lib.h>

#define DEPTH 3

int istrue = 1;

void forktree(const char *cur);

void
hello(int val)
{
    cprintf("Hello, %d!\n", val);
}

void
cont(int val)
{
    cprintf("Continue...\n");
    istrue = 0;
}

void
forkchild(const char *cur, char branch)
{
	char nxt[DEPTH+1];

	if (strlen(cur) >= DEPTH)
		return;

	snprintf(nxt, DEPTH+1, "%s%c", cur, branch);
	if (fork() == 0) {
        struct sigaction sa = {SIG_DFL};
        sys_sigaction(SIGCHLD, &sa);
        if (!strcmp(nxt, "101"))
        {
            sa.sa_handler = hello;
            sys_sigaction(SIGUSR1, &sa);
            sa.sa_handler = cont;
            sys_sigaction(SIGUSR2, &sa);
            cprintf("%04x: Waiting...\n", sys_getenvid());
            while (istrue)
            {
                sys_sigwait(0);
            }
        }
		forktree(nxt);
		exit();
	}
}

void
forktree(const char *cur)
{
	cprintf("%04x: I am '%s'\n", sys_getenvid(), cur);

	forkchild(cur, '0');
	forkchild(cur, '1');
}

void
mourn(int val)
{
    cprintf("You killed my child %04x :(\n", val);
}

void
umain(int argc, char **argv)
{
    struct sigaction sa = {mourn};
    sys_sigaction(SIGCHLD, &sa);
	forktree("");
    while (1)
    {
        int signo;
        sys_sigwait(&signo);
        cprintf("Received %d\n", signo);
    }
}

