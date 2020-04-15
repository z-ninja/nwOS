#ifndef BREE_EXPORT_H
#define BREE_EXPORT_H
#ifdef BREE_HAVE_CONFIG_H
#include <bree_config.h>
#endif
#ifndef BREE_LIBRARY_BUILD
#define BREE_PUBLIC
#define BREE_LOCAL
#else
#ifdef BREE_WINDOWS
#define BREE_PUBLIC __declspec(dllexport)
#define BREE_LOCAL
#elif defined BREE_LINUX
#define BREE_PUBLIC __attribute__ ((visibility ("default")))
#define BREE_LOCAL __attribute__ ((visibility ("hidden")))
#elif defined BREE_MAC_OS_X
#define BREE_PUBLIC __attribute__ ((visibility ("default")))
#define BREE_LOCAL __attribute__ ((visibility ("hidden")))
#else

#endif // BREE_WINDOWS,BREE_LINUX,BREE_MAC_OS_X

#endif // BREE_LIBRARY_BUILD
#endif // BREE_EXPORT_H
