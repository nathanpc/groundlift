/**
 * avahi.c
 * Avahi mDNS abstraction layer.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "avahi.h"

#include <assert.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/simple-watch.h>

/* Private variables. */
static AvahiSimplePoll *m_simple_poll;
static AvahiClient *m_client;
static AvahiServiceBrowser *m_sb;

/* Avahi callbacks. */
static void client_callback(
	AvahiClient *c,
	AvahiClientState state,
	AVAHI_GCC_UNUSED void *userdata);
static void browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void *userdata);
static void resolve_callback(
	AvahiServiceResolver *r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char *name,
	const char *type,
	const char *domain,
	const char *host_name,
	const AvahiAddress *address,
	uint16_t port,
	AvahiStringList *txt,
	AvahiLookupResultFlags flags,
	AVAHI_GCC_UNUSED void *userdata);

/**
 * Initializes the mDNS client.
 *
 * @warning Upon error a call to mdns_free is automatically made.
 *
 * @return MDNS_OK if the initialization was successful.
 *         MDNS_ERR_INIT if an error occurred.
 *
 * @see mdns_free
 */
mdns_err_t mdns_init(void) {
	int error;

	/* Reset all variables. */
	m_simple_poll = NULL;
	m_client = NULL;
	m_sb = NULL;

	/* Allocate main loop object. */
	if (!(m_simple_poll = avahi_simple_poll_new())) {
		fprintf(stderr, "Failed to create simple poll object.\n");

		mdns_free();
		return MDNS_ERR_INIT;
	}

	/* Allocate a new client. */
	m_client = avahi_client_new(avahi_simple_poll_get(m_simple_poll),
								0,
								client_callback,
								NULL,
								&error);

	/* Check if the client object creation succeeded. */
	if (!m_client) {
		fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));

		mdns_free();
		return MDNS_ERR_INIT;
	}

	/* Create the service browser. */
	m_sb = avahi_service_browser_new(m_client,
									 AVAHI_IF_UNSPEC,
									 AVAHI_PROTO_UNSPEC,
									 "_http._tcp",
									 NULL,
									 0,
									 browse_callback,
									 m_client);

	/* Check if the service browser object creation succeeded. */
	if (!m_sb) {
		fprintf(stderr, "Failed to create service browser: %s\n",
				avahi_strerror(avahi_client_errno(m_client)));

		mdns_free();
		return MDNS_ERR_INIT;
	}

	return MDNS_OK;
}

/**
 * Frees up any resources allocated by ourselves.
 */
void mdns_free(void) {
	if (m_sb)
		avahi_service_browser_free(m_sb);

	if (m_client)
		avahi_client_free(m_client);

	if (m_simple_poll)
		avahi_simple_poll_free(m_simple_poll);
}

/**
 * Starts the mDNS event loop.
 *
 * @return MDNS_OK if we got out of the loop without any issues.
 */
mdns_err_t mdns_event_loop(void) {
	avahi_simple_poll_loop(m_simple_poll);
	return MDNS_OK;
}

static void client_callback(AvahiClient *c,
							AvahiClientState state,
							AVAHI_GCC_UNUSED void *userdata) {
	assert(c);

	/* Called whenever the client or server state changes */
	if (state == AVAHI_CLIENT_FAILURE) {
		fprintf(stderr, "Server connection failure: %s\n",
				avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(m_simple_poll);
	}
}

static void browse_callback(AvahiServiceBrowser *b,
							AvahiIfIndex interface,
							AvahiProtocol protocol,
							AvahiBrowserEvent event,
							const char *name,
							const char *type,
							const char *domain,
							AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
							void *userdata) {
	AvahiClient *c = userdata;
	assert(b);

	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
	switch (event) {
		case AVAHI_BROWSER_FAILURE:
			fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
			avahi_simple_poll_quit(m_simple_poll);
			return;
		case AVAHI_BROWSER_NEW:
			fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
			/* We ignore the returned resolver object. In the callback
			   function we free it. If the server is terminated before
			   the callback function is called the server will free
			   the resolver for us. */
			if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, c)))
				fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));
			break;
		case AVAHI_BROWSER_REMOVE:
			fprintf(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
			break;
		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
			break;
	}
}

static void resolve_callback(AvahiServiceResolver *r,
							 AVAHI_GCC_UNUSED AvahiIfIndex interface,
							 AVAHI_GCC_UNUSED AvahiProtocol protocol,
							 AvahiResolverEvent event,
							 const char *name,
							 const char *type,
							 const char *domain,
							 const char *host_name,
							 const AvahiAddress *address,
							 uint16_t port,
							 AvahiStringList *txt,
							 AvahiLookupResultFlags flags,
							 AVAHI_GCC_UNUSED void *userdata) {
	assert(r);

	/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
			break;
		case AVAHI_RESOLVER_FOUND: {
			char a[AVAHI_ADDRESS_STR_MAX];
			char *t;

			fprintf(stderr, "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);

			avahi_address_snprint(a, sizeof(a), address);
			t = avahi_string_list_to_string(txt);
			fprintf(stderr,
					"\t%s:%u (%s)\n"
					"\tTXT=%s\n"
					"\tcookie is %u\n"
					"\tis_local: %i\n"
					"\tour_own: %i\n"
					"\twide_area: %i\n"
					"\tmulticast: %i\n"
					"\tcached: %i\n",
					host_name, port, a,
					t,
					avahi_string_list_get_service_cookie(txt),
					!!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
					!!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
					!!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
					!!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
					!!(flags & AVAHI_LOOKUP_RESULT_CACHED));

			avahi_free(t);
		}
	}

	avahi_service_resolver_free(r);
}
