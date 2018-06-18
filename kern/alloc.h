#ifndef JOS_INC_ALLOC_H
#define JOS_INC_ALLOC_H

typedef long Align; /* for alignment to long boundary */

union header { /* block header */
	struct {
		union header *next; /* next block if on free list */
		union header *prev; /* next block if on free list */
		unsigned size; /* size of this block */
	} s;
	Align x; /* force alignment of blocks */
} __attribute__((packed));

typedef union header Header;

#endif
