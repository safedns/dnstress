#ifndef __PROC_H__
#define __PROC_H__

#include "stdio.h"

#include "statistic.h"

// struct proc_link {

// };

/* transmit stats to a proc_fd */
ssize_t proc_transmit_rstats(int proc_fd, const struct rstats_t *stats);

/* receive stats from proc_fd */
ssize_t proc_obtain_rstats(int proc_fd, struct rstats_t *stats);

#endif /* __PROC_H__ */