/**
 * @file errno_str.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
#ifdef CONFIG_ERRNO_STR
const char *errno_str_get(int err)
{
	switch (-err) {
	case EPERM:
		return "Not permitted";
	case ENOENT:
		return "No such file or directory";
	case ESRCH:
		return "No such context";
	case EINTR:
		return "Interrupted system call";
	case EIO:
		return "I/O error";
	case ENXIO:
		return "No such device or address";
	case E2BIG:
		return "Arg list too long";
	case ENOEXEC:
		return "Exec format error";
	case EBADF:
		return "Bad file number";
	case ECHILD:
		return "No children";
	case EAGAIN:
		return "No more contexts";
	case ENOMEM:
		return "Not enough core";
	case EACCES:
		return "Permission denied";
	case EFAULT:
		return "Bad address";
	case ENOTBLK:
		return "Block device required";
	case EBUSY:
		return "Mount device busy";
	case EEXIST:
		return "File exists";
	case EXDEV:
		return "Cross-device link";
	case ENODEV:
		return "No such device";
	case ENOTDIR:
		return "Not a directory";
	case EISDIR:
		return "Is a directory";
	case EINVAL:
		return "Invalid argument";
	case ENFILE:
		return "File table overflow";
	case EMFILE:
		return "Too many open files";
	case ENOTTY:
		return "Not a typewriter";
	case ETXTBSY:
		return "Text file busy";
	case EFBIG:
		return "File too large";
	case ENOSPC:
		return "No space left on device";
	case ESPIPE:
		return "Illegal seek";
	case EROFS:
		return "Read-only file system";
	case EMLINK:
		return "Too many links";
	case EPIPE:
		return "Broken pipe";
	case EDOM:
		return "Argument too large";
	case ERANGE:
		return "Result too large";
	case ENOMSG:
		return "Unexpected message type";
	case EDEADLK:
		return "Resource deadlock avoided";
	case ENOLCK:
		return "No locks available";
	case ENOSTR:
		return "STREAMS device required";
	case ENODATA:
		return "Missing expected message data";
	case ETIME:
		return "STREAMS timeout occurred";
	case ENOSR:
		return "Insufficient memory";
	case EPROTO:
		return "Generic STREAMS error";
	case EBADMSG:
		return "Invalid STREAMS message";
	case ENOSYS:
		return "Function not implemented";
	case ENOTEMPTY:
		return "Directory not empty";
	case ENAMETOOLONG:
		return "File name too long";
	case ELOOP:
		return "Too many levels of symbolic links";
	case EOPNOTSUPP:
		return "Operation not supported on socket";
	case EPFNOSUPPORT:
		return "Protocol family not supported";
	case ECONNRESET:
		return "Connection reset by peer";
	case ENOBUFS:
		return "No buffer space available";
	case EAFNOSUPPORT:
		return "Addr family not supported";
	case EPROTOTYPE:
		return "Protocol wrong type for socket";
	case ENOTSOCK:
		return "Socket operation on non-socket";
	case ENOPROTOOPT:
		return "Protocol not available";
	case ESHUTDOWN:
		return "Can't send after socket shutdown";
	case ECONNREFUSED:
		return "Connection refused";
	case EADDRINUSE:
		return "Address already in use";
	case ECONNABORTED:
		return "Software caused connection abort";
	case ENETUNREACH:
		return "Network is unreachable";
	case ENETDOWN:
		return "Network is down";
	case ETIMEDOUT:
		return "Connection timed out";
	case EHOSTDOWN:
		return "Host is down";
	case EHOSTUNREACH:
		return "No route to host";
	case EINPROGRESS:
		return "Operation now in progress";
	case EALREADY:
		return "Operation already in progress";
	case EDESTADDRREQ:
		return "Destination address required";
	case EMSGSIZE:
		return "Message size";
	case EPROTONOSUPPORT:
		return "Protocol not supported";
	case ESOCKTNOSUPPORT:
		return "Socket type not supported";
	case EADDRNOTAVAIL:
		return "Can't assign requested address";
	case ENETRESET:
		return "Network dropped connection on reset";
	case EISCONN:
		return "Socket is already connected";
	case ENOTCONN:
		return "Socket is not connected";
	case ETOOMANYREFS:
		return "Too many references: can't splice";
	case ENOTSUP:
		return "Unsupported value";
	case EILSEQ:
		return "Illegal byte sequence";
	case EOVERFLOW:
		return "Value overflow";
	case ECANCELED:
		return "Operation canceled";
	default:
		return "?";
	}
}
#else
const char *errno_str_get(int err)
{
	return "ERRNO or ?";
}
#endif