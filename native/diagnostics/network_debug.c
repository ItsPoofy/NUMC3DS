#define NUMC3DS_NETWORK_DIAGNOSTICS 1
#include "network_debug.h"
#include "../core/state.h"

#define NET_LOG_BUFFER_BYTES 256u
#define NET_LOG_HEX_BYTES 256u

static char net_log_buffer[NET_LOG_BUFFER_BYTES];
static u32 net_log_length;
static int net_log_active_level = NET_LOG_OFF;
static int net_log_line_open;

static void net_log_put(const char *text, u32 length)
{
    u32 index;
    if (!text) return;
    for (index = 0; index < length && net_log_length + 1u < NET_LOG_BUFFER_BYTES; ++index) {
        net_log_buffer[net_log_length++] = text[index];
    }
}

static void net_log_put_cstr(const char *text)
{
    u32 length = 0;
    if (!text) return;
    while (text[length]) ++length;
    net_log_put(text, length);
}

static void net_log_put_dec(u32 value)
{
    char digits[11];
    u32 count = 0;
    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value);
    while (count) net_log_put(&digits[--count], 1u);
}

static void net_log_put_hex8(u32 value)
{
    static const char digits[] = "0123456789ABCDEF";
    char out[8];
    int index;
    for (index = 7; index >= 0; --index) {
        out[index] = digits[value & 15u];
        value >>= 4;
    }
    net_log_put(out, 8u);
}

static void net_log_put_hex2(u8 value)
{
    static const char digits[] = "0123456789ABCDEF";
    char out[2];
    out[0] = digits[(value >> 4) & 15u];
    out[1] = digits[value & 15u];
    net_log_put(out, 2u);
}

static void net_log_key(const char *key)
{
    net_log_put(" ", 1u);
    net_log_put_cstr(key ? key : "?");
    net_log_put("=", 1u);
}

void net_log_set_level(int level)
{
    if (level < NET_LOG_OFF) level = NET_LOG_OFF;
    if (level > NET_LOG_VERBOSE) level = NET_LOG_VERBOSE;
    net_log_active_level = level;
}

int net_log_level(void)
{
    return net_log_active_level;
}

int net_log_enabled(int level)
{
    return net_log_active_level >= level;
}

void net_log_open(int level, const char *tag, const char *message)
{
    net_log_line_open = 0;
    if (net_log_active_level < level) return;
    net_log_length = 0;
    net_log_put_cstr("NuMC3DS net");
    net_log_put(" ", 1u);
    net_log_put_cstr(tag ? tag : "?");
    if (message) {
        net_log_put(" ", 1u);
        net_log_put_cstr(message);
    }
    net_log_line_open = 1;
}

void net_log_hex(const char *key, u32 value)
{
    if (!net_log_line_open) return;
    net_log_key(key);
    net_log_put("0x", 2u);
    net_log_put_hex8(value);
}

void net_log_dec(const char *key, u32 value)
{
    if (!net_log_line_open) return;
    net_log_key(key);
    net_log_put_dec(value);
}

void net_log_signed(const char *key, s32 value)
{
    if (!net_log_line_open) return;
    net_log_key(key);
    if (value < 0) {
        net_log_put("-", 1u);
        net_log_put_dec((u32)(-(value + 1)) + 1u);
    } else {
        net_log_put_dec((u32)value);
    }
}

void net_log_flag(const char *key, int value)
{
    if (!net_log_line_open) return;
    net_log_key(key);
    net_log_put(value ? "true" : "false", value ? 4u : 5u);
}

void net_log_text(const char *key, const char *value)
{
    if (!net_log_line_open) return;
    net_log_key(key);
    net_log_put_cstr(value ? value : "");
}

void net_log_ipv4(const char *key, u32 address_be, u16 port_be)
{
    if (!net_log_line_open) return;
    net_log_key(key);
    net_log_put_dec((address_be >> 24) & 0xffu);
    net_log_put(".", 1u);
    net_log_put_dec((address_be >> 16) & 0xffu);
    net_log_put(".", 1u);
    net_log_put_dec((address_be >> 8) & 0xffu);
    net_log_put(".", 1u);
    net_log_put_dec(address_be & 0xffu);
    net_log_put(":", 1u);
    net_log_put_dec(port_be);
}

void net_log_bytes(const char *key, const void *data, u32 length)
{
    const u8 *bytes = (const u8 *)data;
    u32 index;
    u32 capped;
    if (!net_log_line_open) return;
    net_log_key(key);
    if (!bytes) {
        net_log_put("<null>", 6u);
        return;
    }
    capped = length < NET_LOG_HEX_BYTES ? length : NET_LOG_HEX_BYTES;
    for (index = 0; index < capped; ++index) net_log_put_hex2(bytes[index]);
    if (capped < length) net_log_put("..", 2u);
}

void net_log_close(void)
{
    if (!net_log_line_open) return;
    net_log_put("\n", 1u);
    if (s && s->host.debug_string) {
        s->host.debug_string(net_log_buffer, net_log_length);
    }
    net_log_line_open = 0;
}
