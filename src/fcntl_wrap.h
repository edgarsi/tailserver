
#if defined(HAVE_FCNTL_NONBLOCK) || defined(HAVE_FCNTL_NDELAY)
#include <fcntl.h>
#if !defined(O_NONBLOCK) || !defined(O_NDELAY)
#include <unistd.h>
#endif
/* If a system has neither O_NONBLOCK nor NDELAY, upgrade is recommended :P */
/* Not having fcntl() is too archaic as well but I do support that. */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif
#endif

