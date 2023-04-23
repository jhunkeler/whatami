#ifndef WHATAMI_X86_H
#define WHATAMI_X86_H
#if defined(__x86_64__) || defined(__i386__)

#ifndef bit_HTT
// Hyperthreading
#define bit_HTT (1 << 28)
#endif
// Virtualization
#define bit_VRT (1 << 31)

#endif // x86_64 || i386
#endif //WHATAMI_X86_H
