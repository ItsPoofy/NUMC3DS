#ifndef NUMC3DS_NETWORK_DEBUG_H
#define NUMC3DS_NETWORK_DEBUG_H

#include "../core/rt.h"

/* Structured logging for the MCPE online module. Every line is emitted
   through the host debug output (the Azahar emulated debug log) with a
   "NuMC3DS net" prefix, a tag, and key=value fields, e.g.

       NuMC3DS net transport connect type=0x0001 kind=ipv4 addr=192.168.1.7:19132

   Levels:
       NET_LOG_OFF      no output
       NET_LOG_INFO     lifecycle only (install, connect, session, errors)
       NET_LOG_VERBOSE  adds per-refresh/per-socket/per-record detail */

enum {
    NET_LOG_OFF = 0,
    NET_LOG_INFO = 1,
    NET_LOG_VERBOSE = 2
};

void net_log_set_level(int level);
int net_log_level(void);
int net_log_enabled(int level);

/* Open a line. No-op when `level` is above the active level, in which case
   the following field calls and net_log_close() do nothing. */
void net_log_open(int level, const char *tag, const char *message);
void net_log_hex(const char *key, u32 value);
void net_log_dec(const char *key, u32 value);
void net_log_signed(const char *key, s32 value);
void net_log_flag(const char *key, int value);
void net_log_text(const char *key, const char *value);
void net_log_ipv4(const char *key, u32 address_be, u16 port_be);
void net_log_bytes(const char *key, const void *data, u32 length);
void net_log_close(void);

#ifndef NUMC3DS_NETWORK_DIAGNOSTICS
#define net_log_set_level(level) ((void)sizeof(level))
#define net_log_level() NET_LOG_OFF
#define net_log_enabled(level) ((void)sizeof(level), 0)
#define net_log_open(level, tag, message) \
    do { (void)sizeof(level); (void)sizeof(tag); (void)sizeof(message); } while (0)
#define net_log_hex(key, value) do { (void)sizeof(key); (void)sizeof(value); } while (0)
#define net_log_dec(key, value) do { (void)sizeof(key); (void)sizeof(value); } while (0)
#define net_log_signed(key, value) do { (void)sizeof(key); (void)sizeof(value); } while (0)
#define net_log_flag(key, value) do { (void)sizeof(key); (void)sizeof(value); } while (0)
#define net_log_text(key, value) do { (void)sizeof(key); (void)sizeof(value); } while (0)
#define net_log_ipv4(key, address, port) \
    do { (void)sizeof(key); (void)sizeof(address); (void)sizeof(port); } while (0)
#define net_log_bytes(key, data, length) \
    do { (void)sizeof(key); (void)sizeof(data); (void)sizeof(length); } while (0)
#define net_log_close() ((void)0)
#endif

#endif
