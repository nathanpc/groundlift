/**
 * mdnscommon.h
 * mDNS common abstraction layer.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _MDNSCOMMON_H
#define _MDNSCOMMON_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* mDNS return error codes. */
typedef enum {
	MDNS_OK = 0,
	MDNS_ERR_INIT,
	MDNS_ERR_UNKNOWN
} mdns_err_t;

#ifdef __cplusplus
}
#endif

#endif /* _MDNSCOMMON_H */
