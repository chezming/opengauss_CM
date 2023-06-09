/* -------------------------------------------------------------------------
 *
 * elog.h
 *	  openGauss error reporting/logging definitions.
 *
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/elog.h
 *
 * -------------------------------------------------------------------------
 */
#ifndef ELOG_H
#define ELOG_H

#include <setjmp.h>
#ifdef __GNUC__
#include <cxxabi.h>
#endif /* __GNUC__ */

#include "c.h"
#include "gs_threadlocal.h"
#include "gstrace/gstrace_infra.h"
#include "utils/be_module.h"
#include "utils/syscall_lock.h"
#include "securec.h"
#include "securec_check.h"

/* Error level codes */
#define DEBUG5                                 \
    10 /* Debugging messages, in categories of \
        * decreasing detail. */
#define DEBUG4 11
#define DEBUG3 12
#define DEBUG2 13
#define DEBUG1 14 /* used by GUC debug_* variables */
#define LOG                                         \
    15 /* Server operational messages; sent only to \
        * server log by default. */
#define COMMERROR                                    \
    16 /* Client communication problems; same as LOG \
        * for server reporting, but never sent to    \
        * client. */
#define INFO                                          \
    17 /* Messages specifically requested by user (eg \
        * VACUUM VERBOSE output); always sent to      \
        * client regardless of client_min_messages,   \
        * but by default not sent to server log. */
#define NOTICE                                        \
    18 /* Helpful messages to users about query       \
        * operation; sent to client and server log by \
        * default. */
#define WARNING                                      \
    19 /* Warnings.  NOTICE is for expected messages \
        * like implicit sequence creation by SERIAL. \
        * WARNING is for unexpected messages. */
#define ERROR                                       \
    20 /* user error - abort transaction; return to \
        * known state */
#define VERBOSEMESSAGE                                  \
    9 /* indicates to show verbose info for CN and DNs; \
       * for DNs means to send info back to CN */
/* Save ERROR value in PGERROR so it can be restored when Win32 includes
 * modify it.  We have to use a constant rather than ERROR because macros
 * are expanded only when referenced outside macros.
 */
#ifdef WIN32
#define PGERROR 20
#endif
#define FATAL 21 /* fatal error - abort process */
#define PANIC 22 /* take down the other backends with me */

/* MAKE_SQLSTATE('P', '1', '0' , '0', '0')=96 */
#define CUSTOM_ERRCODE_P1 96

/* macros for representing SQLSTATE strings compactly */
#define PGSIXBIT(ch) (((ch) - '0') & 0x3F)
#define PGUNSIXBIT(val) (((val)&0x3F) + '0')

#define MAKE_SQLSTATE(ch1, ch2, ch3, ch4, ch5) \
    (PGSIXBIT(ch1) + (PGSIXBIT(ch2) << 6) + (PGSIXBIT(ch3) << 12) + (PGSIXBIT(ch4) << 18) + (PGSIXBIT(ch5) << 24))

/* These macros depend on the fact that '0' becomes a zero in SIXBIT */
#define ERRCODE_TO_CATEGORY(ec) ((ec) & ((1 << 12) - 1))
#define ERRCODE_IS_CATEGORY(ec) (((ec) & ~((1 << 12) - 1)) == 0)

/* Which __func__ symbol do we have, if any? */
#ifdef HAVE_FUNCNAME__FUNC
#define PG_FUNCNAME_MACRO __func__
#else
#ifdef HAVE_FUNCNAME__FUNCTION
#define PG_FUNCNAME_MACRO __FUNCTION__
#else
#define PG_FUNCNAME_MACRO NULL
#endif
#endif

#define TEXTDOMAIN NULL

#ifdef PC_LINT
#define errcode(sqlerrcode) (1 == (int)(sqlerrcode))
#else
#if !defined(WIN32)
extern int errcode(int sqlerrcode);
#endif
#endif
extern int errcode_for_file_access(void);
extern int errcode_for_socket_access(void);

/* set error module */
extern int errmodule(ModuleId id);
extern const char* mask_encrypted_key(const char* query_string, int str_len);
extern char* maskPassword(const char* query_string);

#define MASK_PASSWORD_START(mask_string, query_string) \
    do {                                               \
        mask_string = maskPassword(query_string);      \
        if (NULL == mask_string)                       \
            mask_string = (char*)query_string;         \
    } while (0)

#define MASK_PASSWORD_END(mask_string, query_string) \
    do {                                             \
        if (mask_string != query_string)             \
            selfpfree(mask_string);                  \
    } while (0)

extern int errmsg(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errmsg_internal(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errmsg_plural(const char* fmt_singular, const char* fmt_plural, unsigned long n, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 4))) __attribute__((format(PG_PRINTF_ATTRIBUTE, 2, 4)));

extern int errdetail(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errdetail_internal(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errdetail_log(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errdetail_plural(const char* fmt_singular, const char* fmt_plural, unsigned long n, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 4))) __attribute__((format(PG_PRINTF_ATTRIBUTE, 2, 4)));

extern int errhint(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errcause(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
 *        the supplied arguments. */
    __attribute__((format(printf, 1, 2)));

extern int erraction(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
 *  *        the supplied arguments. */
    __attribute__((format(printf, 1, 2)));

extern int errquery(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errcontext(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int errhidestmt(bool hide_stmt);

extern int errhideprefix(bool hide_prefix);

extern int errposition(int cursorpos);

extern int internalerrposition(int cursorpos);
extern int internalerrquery(const char* query);
extern int ErrOutToClient(bool outToClient);
extern int geterrcode(void);
extern int geterrposition(void);
extern int getinternalerrposition(void);

extern void save_error_message(void);

extern int handle_in_client(bool handle);
extern int ignore_interrupt(bool ignore);


/* Support for constructing error strings separately from ereport() calls */

extern void pre_format_elog_string(int errnumber, const char* domain);
extern char* format_elog_string(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

/* Support for attaching context information to error reports */

typedef struct ErrorContextCallback {
    struct ErrorContextCallback* previous;
    void (*callback)(void* arg);
    void* arg;
} ErrorContextCallback;


/* ----------
 * API for catching ereport(ERROR) exits.  Use these macros like so:
 *
 *		PG_TRY();
 *		{
 *			... code that might throw ereport(ERROR) ...
 *		}
 *		PG_CATCH();
 *		{
 *			... error recovery code ...
 *		}
 *		PG_END_TRY();
 *
 * (The braces are not actually necessary, but are recommended so that
 * pg_indent will indent the construct nicely.)  The error recovery code
 * can optionally do PG_RE_THROW() to propagate the same error outwards.
 *
 * Note: while the system will correctly propagate any new ereport(ERROR)
 * occurring in the recovery section, there is a small limit on the number
 * of levels this will work for.  It's best to keep the error recovery
 * section simple enough that it can't generate any new errors, at least
 * not before popping the error stack.
 *
 * Note: an ereport(FATAL) will not be caught by this construct; control will
 * exit straight through proc_exit().  Therefore, do NOT put any cleanup
 * of non-process-local resources into the error recovery section, at least
 * not without taking thought for what will happen during ereport(FATAL).
 * The PG_ENSURE_ERROR_CLEANUP macros provided by storage/ipc.h may be
 * helpful in such cases.
 *
 * Note: Don't execute statements such as break, continue, goto, or return in
 * PG_TRY. If you need to use these statements, you must recovery
 * PG_exception_stack first.
 * ----------
 */
#define PG_TRY()                                                                       \
    do {                                                                               \
        sigjmp_buf* save_exception_stack = t_thrd.log_cxt.PG_exception_stack;          \
        ErrorContextCallback* save_context_stack = t_thrd.log_cxt.error_context_stack; \
        sigjmp_buf local_sigjmp_buf;                                                   \
        int tryCounter, *oldTryCounter = NULL;                                         \
        if (sigsetjmp(local_sigjmp_buf, 0) == 0) {                                     \
            t_thrd.log_cxt.PG_exception_stack = &local_sigjmp_buf;                     \
        oldTryCounter = gstrace_tryblock_entry(&tryCounter)

#define PG_CATCH()                                                \
    }                                                             \
    else                                                          \
    {                                                             \
        t_thrd.log_cxt.PG_exception_stack = save_exception_stack; \
        t_thrd.log_cxt.error_context_stack = save_context_stack;  \
        gstrace_tryblock_exit(true, oldTryCounter)

#define PG_END_TRY()                                          \
    }                                                         \
    t_thrd.log_cxt.PG_exception_stack = save_exception_stack; \
    t_thrd.log_cxt.error_context_stack = save_context_stack;  \
    gstrace_tryblock_exit(false, oldTryCounter);              \
    }                                                         \
    while (0)

// ADIO means async direct io
#define ADIO_RUN() if (g_instance.attr.attr_storage.enable_adio_function) {

#define ADIO_ELSE() \
    }               \
    else            \
    {

#define ADIO_END() }

// BFIO means buffer io
#define BFIO_RUN() if (!g_instance.attr.attr_storage.enable_adio_function) {

#define BFIO_ELSE() \
    }               \
    else            \
    {

#define BFIO_END() }

#define ADIO_LOG_DB(A)                                     \
    do {                                                   \
        if (u_sess->attr.attr_storage.enable_adio_debug) { \
            A;                                             \
        }                                                  \
    } while (0)

/*
 * gcc understands __attribute__((noreturn)); for other compilers, insert
 * a useless exit() call so that the compiler gets the point.
 */
#ifdef __GNUC__
#define PG_RE_THROW() pg_re_throw()
#else
#define PG_RE_THROW() (pg_re_throw(), exit(1))  // no need to change exit to pthread_exit
#endif

/* Stuff that error handlers might want to use */

/*
 * ErrorData holds the data accumulated during any one ereport() cycle.
 * Any non-NULL pointers must point to palloc'd data.
 * (The const pointers are an exception; we assume they point at non-freeable
 * constant strings.)
 */
typedef struct ErrorData {
    int elevel;            /* error level */
    bool output_to_server; /* will report to server log? */
    bool output_to_client; /* will report to client? */
    bool handle_in_client; /* true to report to client and also handle in client */
    bool show_funcname;    /* true to force funcname inclusion */
    bool hide_stmt;        /* true to prevent STATEMENT: inclusion */
    bool hide_prefix;      /* true to prevent line prefix inclusion */
    char* filename;        /* __FILE__ of ereport() call */
    int lineno;            /* __LINE__ of ereport() call */
    char* funcname;        /* __func__ of ereport() call */
    const char* domain;    /* message domain */
    int sqlerrcode;        /* encoded ERRSTATE */
    ModuleId mod_id;       /* which module */
    char* message;         /* primary error message */
    char* detail;          /* detail error message */
    char* detail_log;      /* detail error message for server log only */
    char* hint;            /* hint message */
    char* context;         /* context message */
    int cursorpos;         /* cursor index into query string */
    int internalpos;       /* cursor index into internalquery */
    char* internalquery;   /* text of internally-generated query */
    int saved_errno;       /* errno at entry */
    char* backtrace_log;   /* the buffer for backtrace */
    int internalerrcode;   /* mppdb internal encoded */
    bool verbose;          /* the flag to indicate VACUUM FULL VERBOSE/ANALYZE VERBOSE message */
    bool ignore_interrupt; /* true to ignore interrupt when writing server log */
    char* cause;
    char* action;
} ErrorData;

/* The error data from remote */
typedef struct RemoteErrorData {
    int internalerrcode; /* mppdb internal encoded */
    char* errorfuncname; /* __func__ of ereport() call */
    char* filename;      /* __FILE__ of ereport() call */
    int lineno;          /* __LINE__ of ereport() call */
    ModuleId mod_id;
} RemoteErrorData;

extern int combiner_errdata(RemoteErrorData* pErrData);
extern char *Geterrmsg(void);
extern void EmitErrorReport(void);
extern void stream_send_message_to_server_log(void);
extern void stream_send_message_to_consumer(void);
extern ErrorData* CopyErrorData(void);
extern void UpdateErrorData(ErrorData* edata, ErrorData* newData);
extern void FreeErrorData(ErrorData* edata);
extern void FlushErrorState(void);
extern void FlushErrorStateWithoutDeleteChildrenContext(void);
extern void ReThrowError(ErrorData* edata) __attribute__((noreturn));
extern void pg_re_throw(void) __attribute__((noreturn));
extern void PgRethrowAsFatal(void);

/* GUC-configurable parameters */

typedef enum {
    PGERROR_TERSE,   /* single-line error messages */
    PGERROR_DEFAULT, /* recommended style */
    PGERROR_VERBOSE  /* all the facts, ma'am */
} PGErrorVerbosity;

/* Log destination bitmap */
#define LOG_DESTINATION_STDERR 1
#define LOG_DESTINATION_SYSLOG 2
#define LOG_DESTINATION_EVENTLOG 4
#define LOG_DESTINATION_CSVLOG 8
#define LOG_DESTINATION_QUERYLOG 16
#define LOG_DESTINATION_ASPLOG	 32

/* Other exported functions */
extern void DebugFileOpen(void);
extern char* unpack_sql_state(int sql_state);
extern bool in_error_recursion_trouble(void);

#ifdef HAVE_SYSLOG
extern void set_syslog_parameters(const char* ident, int facility);
#endif

#ifndef WIN32
/*
 * @Description: according to module logging rules,
 *               check this module logging is enable or disable.
 * @IN elevel: error level
 * @IN mod_id: module id
 * @Return: true, enable server log to write; false, disable server logging.
 * @See also: send_message_to_server_log()
 */
extern inline bool is_errmodule_enable(int elevel, ModuleId mod_id)
{
    return (elevel >= LOG || module_logging_is_on(mod_id));
}

extern inline int defence_errlevel(void)
{
#ifdef USE_ASSERT_CHECKING
    return PANIC;
#else
    return ERROR;
#endif
}

#endif

/*
 * Write errors to stderr (or by equal means when stderr is
 * not available). Used before ereport/elog can be used
 * safely (memory context, GUC load etc)
 */
extern void write_stderr(const char* fmt, ...)
    /* This extension allows gcc to check the format string for consistency with
       the supplied arguments. */
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern void getElevelAndSqlstate(int* eLevel, int* sqlState);

void freeSecurityFuncSpace(char* charList, ...);

extern void SimpleLogToServer(int elevel, bool silent, const char* fmt, ...)
    __attribute__((format(PG_PRINTF_ATTRIBUTE, 3, 4)));

#endif /* ELOG_H */
