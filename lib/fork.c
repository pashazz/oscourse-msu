// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;addr=addr;
	uint32_t err = utf->utf_err;err=err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

    if (!(uvpt[PGNUM(addr)] & (PTE_W|PTE_COW))) panic("pgfault: page is not COW!\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

    void *dst = (void *) ROUNDDOWN((uintptr_t) addr, PGSIZE);
    int error = sys_page_alloc(0, (void *) PFTEMP, PTE_W | PTE_U);
    if (error) panic("pgfault: sys_page_alloc error!\n");
    memmove((void *) PFTEMP, dst, PGSIZE);
    error = sys_page_map(0, (void *) PFTEMP, 0, dst, PTE_P | PTE_W | PTE_U);
    if (error) panic("pgfault: sys_page_map error!\n");
    error = sys_page_unmap(0, (void *) PFTEMP);
    if (error) panic("pgfault: sys_page_unmap error!\n");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
    int err;

    void *va = (void *) (pn << PGSHIFT);
    int perm = PTE_U | PTE_P;
    if (uvpt[pn] & PTE_SHARE)
    {
        err = sys_page_map(0, va, envid, va, PGOFF(va) | PTE_SYSCALL);
        if (err) return err;
    }
    else
    {
        if (uvpt[pn] & (PTE_W | PTE_COW)) perm |= PTE_COW;
        err = sys_page_map(0, va, envid, va, perm);
        if (err) return err;
        err = sys_page_map(0, va, 0, va, perm);
        if (err) return err;
    }
    return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
    set_pgfault_handler(pgfault);
    envid_t envid = sys_exofork();
    if(envid < 0) return envid;
    if(!envid)
    {
        thisenv = envs + ENVX(sys_getenvid());
        return 0;
    }
    for (intptr_t i = PGNUM(UTEXT); i != PGNUM(UTOP); ++i)
    {
        if (!(uvpd[PDX(i << PGSHIFT)] & PTE_P)) continue;
        if (!(uvpt[i] & PTE_P)) continue;
        if ((i << PGSHIFT) < UXSTACKTOP - PGSIZE) duppage(envid, i);
    }
    int err = sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_U | PTE_W);
    if (err) return err;
    err = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
    if (err) return err;
    err = sys_env_set_status(envid, ENV_RUNNABLE);
    if (err) return err;
    return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
