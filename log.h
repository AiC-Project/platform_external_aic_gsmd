#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define  DEBUG  1

#ifndef HOST_BUILD

#define LOG_TAG "GSMd"
#include <utils/Log.h>

#if DEBUG
#  define  D(...)   do { RLOGD(__VA_ARGS__ ); } while (0)
#  define  R(...)   do { RLOGD(__VA_ARGS__ ); } while (0)
#else
#  define  D(...)   ((void)0)
#  define  R(...)   ((void)0)
#endif
#else
#if DEBUG
#  define  D(...)   do { fprintf( stderr, __VA_ARGS__ ); fprintf(stderr, "\n");} while (0)
#  define  R(...)   do { fprintf( stderr, __VA_ARGS__ ); fprintf(stderr, "\n");} while (0)
#else
#  define  D(...)   ((void)0)
#  define  R(...)   ((void)0)
#endif
#endif


