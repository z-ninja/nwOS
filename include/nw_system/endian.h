#ifndef NW_ENDIAN_H
#define NW_ENDIAN_H

#ifdef NW_LINUX
#define NW_LITTLE_ENDIAN
#elif defined (NW_WINDOWS)
#define NW_LITTLE_ENDIAN
#elif defined(NW_MAC_OS_X)
#define NW_LITTLE_ENDIAN
#endif

#endif // NW_ENDIAN_H
