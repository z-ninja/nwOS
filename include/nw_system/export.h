#ifndef NW_SYSTEM_EXPORT_H
#define NW_SYSTEM_EXPORT_H

#ifndef NW_SYSTEM_LIBRARY_BUILD
#define NW_PUBLIC
#define NW_LOCAL
#else
#ifdef NW_WINDOWS
#define NW_PUBLIC __declspec(dllexport)
#define NW_LOCAL
#elif defined NW_LINUX
#define NW_PUBLIC __attribute__ ((visibility ("default")))
#define NW_LOCAL __attribute__ ((visibility ("hidden")))
#elif defined NW_MAC_OS_X
#define NW_PUBLIC __attribute__ ((visibility ("default")))
#define NW_LOCAL __attribute__ ((visibility ("hidden")))
#else

#endif // NW_WINDOWS,NW_LINUX,NW_MAC_OS_X

#endif // NW_UI_LIBRARY_BUILD
#endif // NW_SYSTEM_EXPORT_H
