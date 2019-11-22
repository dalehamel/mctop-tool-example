/* Generated by the Systemtap dtrace wrapper */

#include <sys/sdt.h>

/* MEMCACHED_CONN_ALLOCATE ( int connid ) */
#define MEMCACHED_CONN_ALLOCATE(arg1) \
DTRACE_PROBE1 (memcached, conn__allocate, arg1)

/* MEMCACHED_CONN_RELEASE ( int connid ) */
#define MEMCACHED_CONN_RELEASE(arg1) \
DTRACE_PROBE1 (memcached, conn__release, arg1)

/* MEMCACHED_CONN_CREATE ( void *ptr ) */
#define MEMCACHED_CONN_CREATE(arg1) \
DTRACE_PROBE1 (memcached, conn__create, arg1)

/* MEMCACHED_CONN_DESTROY ( void *ptr ) */
#define MEMCACHED_CONN_DESTROY(arg1) \
DTRACE_PROBE1 (memcached, conn__destroy, arg1)

/* MEMCACHED_CONN_DISPATCH ( int connid, int64_t threadid ) */
#define MEMCACHED_CONN_DISPATCH(arg1, arg2) \
DTRACE_PROBE2 (memcached, conn__dispatch, arg1, arg2)

/* MEMCACHED_SLABS_ALLOCATE ( int size, int slabclass, int slabsize, void* ptr ) */
#define MEMCACHED_SLABS_ALLOCATE(arg1, arg2, arg3, arg4) \
DTRACE_PROBE4 (memcached, slabs__allocate, arg1, arg2, arg3, arg4)

/* MEMCACHED_SLABS_ALLOCATE_FAILED ( int size, int slabclass ) */
#define MEMCACHED_SLABS_ALLOCATE_FAILED(arg1, arg2) \
DTRACE_PROBE2 (memcached, slabs__allocate__failed, arg1, arg2)

/* MEMCACHED_SLABS_SLABCLASS_ALLOCATE ( int slabclass ) */
#define MEMCACHED_SLABS_SLABCLASS_ALLOCATE(arg1) \
DTRACE_PROBE1 (memcached, slabs__slabclass__allocate, arg1)

/* MEMCACHED_SLABS_SLABCLASS_ALLOCATE_FAILED ( int slabclass ) */
#define MEMCACHED_SLABS_SLABCLASS_ALLOCATE_FAILED(arg1) \
DTRACE_PROBE1 (memcached, slabs__slabclass__allocate__failed, arg1)

/* MEMCACHED_SLABS_FREE ( int size, int slabclass, void* ptr ) */
#define MEMCACHED_SLABS_FREE(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, slabs__free, arg1, arg2, arg3)

/* MEMCACHED_ASSOC_FIND ( const char *key, int keylen, int depth ) */
#define MEMCACHED_ASSOC_FIND(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, assoc__find, arg1, arg2, arg3)

/* MEMCACHED_ASSOC_INSERT ( const char *key, int keylen ) */
#define MEMCACHED_ASSOC_INSERT(arg1, arg2) \
DTRACE_PROBE2 (memcached, assoc__insert, arg1, arg2)

/* MEMCACHED_ASSOC_DELETE ( const char *key, int keylen ) */
#define MEMCACHED_ASSOC_DELETE(arg1, arg2) \
DTRACE_PROBE2 (memcached, assoc__delete, arg1, arg2)

/* MEMCACHED_ITEM_LINK ( const char *key, int keylen, int size ) */
#define MEMCACHED_ITEM_LINK(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, item__link, arg1, arg2, arg3)

/* MEMCACHED_ITEM_UNLINK ( const char *key, int keylen, int size ) */
#define MEMCACHED_ITEM_UNLINK(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, item__unlink, arg1, arg2, arg3)

/* MEMCACHED_ITEM_REMOVE ( const char *key, int keylen, int size ) */
#define MEMCACHED_ITEM_REMOVE(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, item__remove, arg1, arg2, arg3)

/* MEMCACHED_ITEM_UPDATE ( const char *key, int keylen, int size ) */
#define MEMCACHED_ITEM_UPDATE(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, item__update, arg1, arg2, arg3)

/* MEMCACHED_ITEM_REPLACE ( const char *oldkey, int oldkeylen, int oldsize, const char *newkey, int newkeylen, int newsize ) */
#define MEMCACHED_ITEM_REPLACE(arg1, arg2, arg3, arg4, arg5, arg6) \
DTRACE_PROBE6 (memcached, item__replace, arg1, arg2, arg3, arg4, arg5, arg6)

/* MEMCACHED_PROCESS_COMMAND_START ( int connid, const void *request, int size ) */
#define MEMCACHED_PROCESS_COMMAND_START(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, process__command__start, arg1, arg2, arg3)

/* MEMCACHED_PROCESS_COMMAND_END ( int connid, const void *response, int size ) */
#define MEMCACHED_PROCESS_COMMAND_END(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, process__command__end, arg1, arg2, arg3)

/* MEMCACHED_COMMAND_GET ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_GET(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__get, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_ADD ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_ADD(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__add, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_SET ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_SET(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__set, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_REPLACE ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_REPLACE(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__replace, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_PREPEND ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_PREPEND(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__prepend, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_APPEND ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_APPEND(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__append, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_TOUCH ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_TOUCH(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__touch, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_CAS ( int connid, const char *key, int keylen, int size, int64_t casid ) */
#define MEMCACHED_COMMAND_CAS(arg1, arg2, arg3, arg4, arg5) \
DTRACE_PROBE5 (memcached, command__cas, arg1, arg2, arg3, arg4, arg5)

/* MEMCACHED_COMMAND_INCR ( int connid, const char *key, int keylen, int64_t val ) */
#define MEMCACHED_COMMAND_INCR(arg1, arg2, arg3, arg4) \
DTRACE_PROBE4 (memcached, command__incr, arg1, arg2, arg3, arg4)

/* MEMCACHED_COMMAND_DECR ( int connid, const char *key, int keylen, int64_t val ) */
#define MEMCACHED_COMMAND_DECR(arg1, arg2, arg3, arg4) \
DTRACE_PROBE4 (memcached, command__decr, arg1, arg2, arg3, arg4)

/* MEMCACHED_COMMAND_DELETE ( int connid, const char *key, int keylen ) */
#define MEMCACHED_COMMAND_DELETE(arg1, arg2, arg3) \
DTRACE_PROBE3 (memcached, command__delete, arg1, arg2, arg3)

