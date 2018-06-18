// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/tsc.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/syscall.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Display a backtrace of the entire stack: one line per frame for all frames in the stack", mon_backtrace },
	{ "echo", "Display a line of text", mon_echo },
	{ "start", "Start timer", mon_timer_start },
	{ "stop", "Stop timer", mon_timer_stop },
	{ "lpp", "List physical pages", mon_list_pages },
	{ "kill", "Send a signal to a process", mon_kill },
	{ "exit", "Exit monitor", mon_exit },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", (uint32_t)_start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n",
            (uint32_t)entry, (uint32_t)entry - KERNTOP);
	cprintf("  etext  %08x (virt)  %08x (phys)\n",
            (uint32_t)etext, (uint32_t)etext - KERNTOP);
	cprintf("  edata  %08x (virt)  %08x (phys)\n",
            (uint32_t)edata, (uint32_t)edata - KERNTOP);
	cprintf("  end    %08x (virt)  %08x (phys)\n",
            (uint32_t)end, (uint32_t)end - KERNTOP);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_echo(int argc, char **argv, struct Trapframe *tf)
{
    for (int i = 1; i < argc; ++i)
    {
        for (int j = 0; j != strlen(argv[i]); ++j)
        {
            cputchar(argv[i][j]);
        }
        if (i != argc - 1) cprintf(" ");
    }
    cprintf("\n");
    return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    cprintf("Stack backtrace:\n");
    uint32_t ebp = read_ebp();
    uint32_t eip = (uint32_t)mon_backtrace;
    while (1)
    {
        struct Eipdebuginfo info;
        debuginfo_eip(eip, &info);
        uint32_t off = eip - (uint32_t)info.eip_fn_addr;
        eip = *((uint32_t *)ebp + 1);
        cprintf("  ebp %08x  eip %08x  args", ebp, eip);
        for (int i = 0; i != 5; ++i)
        {
            cprintf(" %08x", *((uint32_t *)ebp + i + 2));
        }
        cprintf("\n         %s:%d: %.*s+%u\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, off);
        ebp = *(uint32_t *)ebp;
        if (!ebp) break;
    }
	return 0;
}

int
mon_timer_start(int argc, char **argv, struct Trapframe *tf)
{
    timer_start();
    return 0;
}

int
mon_timer_stop(int argc, char **argv, struct Trapframe *tf)
{
    timer_stop();
    return 0;
}

int
mon_list_pages(int argc, char **argv, struct Trapframe *tf)
{
    bool allocated = !!pages[0].pp_ref;
    unsigned start = 1;
    for (size_t i = 1; i <= npages; ++i)
    {
        bool tmp = false;
        if (i != npages) tmp = !!pages[i].pp_ref;
        if (i == npages || tmp != allocated)
        {
            cprintf("%u", start);
            if (start != i) cprintf("..%u", (unsigned) i);
            allocated ? cprintf(" ALLOCATED\n") : cprintf(" FREE\n");
            start = i + 1;
            allocated = tmp;
        }
    }
    return 0;
}

int
mon_kill(int argc, char **argv, struct Trapframe *tf)
{
    int signo;
    int value;
    envid_t envid;
    switch (argc)
    {
    case 3:
        signo = -strtol(argv[1], 0, 10);
        if (signo < 0) break;
        envid = strtol(argv[2], 0, 16);
        if (syscall(SYS_sigqueue, envid, signo, 0, 0, 0)) break;
        return 0;
    case 4:
        signo = -strtol(argv[1], 0, 10);
        if (signo < 0) break;
        value = strtol(argv[2], 0, 10);
        envid = strtol(argv[3], 0, 16);
        if (syscall(SYS_sigqueue, envid, signo, value, 0, 0)) break;
        return 0;
    default:
        break;
    };
    cprintf("Usage: %s -[SIGNO] ([VALUE] [ENVID]) | [ENVID]\n", argv[0]);
    cprintf("Send a signal to a process.\n");
    return 0;
}

int
mon_exit(int argc, char **argv, struct Trapframe *tf)
{
    return -1;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
