#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
void dbg_init(void);
void dbg_printf(const char *format, ...);
#else
static inline void dbg_init()
{
}

static inline void dbg_printf()
{
}
#endif

#endif

