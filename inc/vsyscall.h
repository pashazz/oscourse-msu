#ifndef JOS_INC_VSYSCALL_H
#define JOS_INC_VSYSCALL_H

/* system call numbers */
enum {
	VSYS_gettime,
    VSYS_sigreturn,
	NVSYSCALLS
};

#endif /* !JOS_INC_VSYSCALL_H */
