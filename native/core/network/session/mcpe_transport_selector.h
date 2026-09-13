#ifndef NUMC3DS_MCPE_TRANSPORT_SELECTOR_H
#define NUMC3DS_MCPE_TRANSPORT_SELECTOR_H

#include "../../rt.h"

enum McpeTransportKind {
    MCPE_TRANSPORT_NATIVE = 0,
    MCPE_TRANSPORT_IPV4 = 1
};

int mcpe_transport_selector_install(void);
int mcpe_transport_selector_kind(void);
const char *mcpe_transport_selector_server_address(void);

/* Set by the discovery UI whenever a remote (MCPE LAN) server is present in the
   Join Server list. The LocalWirelessNetwork join handler tries Nintendo UDS
   first; this flag lets us fall through to the selected IPv4 connection instead
   of the Undefined default when that UDS join fails. */
void mcpe_transport_selector_set_remote_available(int available);
int mcpe_transport_selector_remote_available(void);

/* IPv4 endpoint of the discovered MCPE server currently selected in the Join
   Server list. The stock network object keeps its fake UDS address
   (0.0.0.0/10.0.0.1:14), so the RNS2 send adapter substitutes this endpoint. */
void mcpe_transport_selector_set_remote_endpoint(u32 address_be, u16 port_be);
int mcpe_transport_selector_remote_endpoint(u32 *address_be, u16 *port_be);
void mcpe_transport_selector_clear_session(void);

#endif
