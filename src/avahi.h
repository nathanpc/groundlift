/**
 * avahi.h
 * Avahi mDNS abstraction layer.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _AVAHIHELPER_H
#define _AVAHIHELPER_H

#include <stdio.h>
#include <stdlib.h>

#include "mdnscommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization and destruction. */
mdns_err_t mdns_init(void);
mdns_err_t mdns_event_loop(void);
void mdns_free(void);

#ifdef __cplusplus
}
#endif

#endif /* _AVAHIHELPER_H */
