#include "mcpe_resource_pack_compat.h"
#include "../session/mcpe_transport_selector.h"
#include "../../skin/remote_skin_service.h"
#include "../../commands/native_types.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../rt.h"

typedef unsigned long long (*ReadUnsignedInt64Fn)(void *);
typedef void (*ReadStringFn)(NativeGstdString *, void *);
typedef void (*GstdDtorFn)(void *);
typedef void (*StrCtorFn)(void *out, const char *text, void *scratch);
typedef void (*StringAssignFn)(void *dest, const void *src);
typedef void (*ResourcePacksInfoHandlerFn)(void *, void *, void *);
typedef void (*ResourcePacksStackHandlerFn)(void *, void *, void *);
typedef void (*NetworkHandlerSendFn)(void *, void *, void *);

static NuMC3DS_Hook read_u64_hook;
static NuMC3DS_Hook read_string_hook;
static NuMC3DS_Hook resource_packs_info_hook;
static NuMC3DS_Hook resource_packs_stack_hook;

static void send_resource_pack_response(void *handler, void *identifier, u8 status)
{
    void *network_handler;
    struct {
        u32 vtable;
        u8 reliability;
        u8 client_sub_id;
        u8 pad1[2];
        u32 vec_start;
        u32 vec_end;
        u32 vec_cap;
        u8 status;
        u8 pad2[3];
    } resp_pkt;

    if (!handler || !identifier) return;
    network_handler = *(void **)((u8 *)handler + 4u);
    zero(&resp_pkt, sizeof(resp_pkt));
    resp_pkt.vtable = SEAM_ResourcePackClientResponsePacket_vtable;
    resp_pkt.reliability = 2;
    resp_pkt.client_sub_id = 1;
    resp_pkt.status = status;
    ((NetworkHandlerSendFn)SEAM_NetworkHandler_send)(network_handler, identifier, &resp_pkt);
}

static void on_resource_packs_info(void *handler, void *identifier, void *packet)
{
    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) {
        ResourcePacksInfoHandlerFn original = (ResourcePacksInfoHandlerFn)resource_packs_info_hook.trampoline;
        original(handler, identifier, packet);
        return;
    }
    send_resource_pack_response(handler, identifier, 3);
}

static void on_resource_packs_stack(void *handler, void *identifier, void *packet)
{
    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) {
        ResourcePacksStackHandlerFn original = (ResourcePacksStackHandlerFn)resource_packs_stack_hook.trampoline;
        original(handler, identifier, packet);
        return;
    }
    send_resource_pack_response(handler, identifier, 4);
}

static unsigned long long on_read_unsigned_int64(void *stream)
{
    ReadUnsignedInt64Fn original;
    unsigned long long value;
    u32 return_address;
    original = (ReadUnsignedInt64Fn)read_u64_hook.trampoline;
    value = original(stream);
    return_address = (u32)__builtin_return_address(0);
    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4 &&
        (return_address == 0x0046AFA0u || return_address == 0x0046B0E0u)) {
        NativeGstdString content_key;
        zero(&content_key, sizeof(content_key));
        ((ReadStringFn)SEAM_ReadOnlyBinaryStream_getString)(&content_key, stream);
        if (content_key.handle) ((GstdDtorFn)SEAM_StrDtor)(&content_key);
    }
    return value;
}

static int native_string_view(const NativeGstdString *value, const char **bytes, u32 *length)
{
    u32 handle;
    if (!value || !bytes || !length || !(handle = value->handle)) return 0;
    *bytes = (const char *)handle;
    *length = *(const u32 *)(handle - 4u);
    return 1;
}

static void on_read_string(NativeGstdString *output, void *stream)
{
    ReadStringFn original;
    u32 return_address;
    original = (ReadStringFn)read_string_hook.trampoline;
    original(output, stream);
    return_address = (u32)__builtin_return_address(0);
    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4 &&
        return_address == 0x0033AD14u) {
        NativeGstdString skin_data;
        zero(&skin_data, sizeof(skin_data));
        original(&skin_data, stream);
        if (skin_data.handle) {
            const char *raw_bytes = 0;
            u32 raw_len = 0;
            if (native_string_view(&skin_data, &raw_bytes, &raw_len) &&
                (raw_len == 8192u || raw_len == 16384u)) {
                const u8 *uuid = (const u8 *)output - 88u;
                const char *skin_id_chars = 0;
                u32 skin_id_len = 0;
                char remote_name[48];
                native_string_view(output, &skin_id_chars, &skin_id_len);
                if (remote_skin_service_register(uuid, skin_id_chars,
                                                 (const u8 *)raw_bytes, raw_len,
                                                 remote_name, sizeof(remote_name))) {
                    NativeGstdString new_name;
                    u32 scratch = 0;
                    zero(&new_name, sizeof(new_name));
                    ((StrCtorFn)SEAM_StrCtor)(&new_name, remote_name, &scratch);
                    ((StringAssignFn)SEAM_StrAssign)(output, &new_name);
                    if (new_name.handle) ((GstdDtorFn)SEAM_StrDtor)(&new_name);
                }
            }
            ((GstdDtorFn)SEAM_StrDtor)(&skin_data);
        }
    }
}

int mcpe_resource_pack_compat_install(void)
{
    zero(&read_u64_hook, sizeof(read_u64_hook));
    read_u64_hook.target = SEAM_ReadOnlyBinaryStream_getUnsignedInt64;
    read_u64_hook.replacement = (u32)on_read_unsigned_int64;
    read_u64_hook.expected[0] = 0xE52DE004u;
    read_u64_hook.expected[1] = 0xE5901000u;
    if (s->host.install_hook(&read_u64_hook)) return -81;
    zero(&read_string_hook, sizeof(read_string_hook));
    read_string_hook.target = SEAM_ReadOnlyBinaryStream_getString;
    read_string_hook.replacement = (u32)on_read_string;
    read_string_hook.expected[0] = 0xE92D4FF0u;
    read_string_hook.expected[1] = 0xE3A06000u;
    if (s->host.install_hook(&read_string_hook)) return -82;
    zero(&resource_packs_info_hook, sizeof(resource_packs_info_hook));
    resource_packs_info_hook.target = SEAM_ClientNetworkHandler_handleResourcePacksInfoPacket;
    resource_packs_info_hook.replacement = (u32)on_resource_packs_info;
    resource_packs_info_hook.expected[0] = 0xE92D4010u;
    resource_packs_info_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&resource_packs_info_hook)) return -83;
    zero(&resource_packs_stack_hook, sizeof(resource_packs_stack_hook));
    resource_packs_stack_hook.target = SEAM_ClientNetworkHandler_handleResourcePacksStackPacket;
    resource_packs_stack_hook.replacement = (u32)on_resource_packs_stack;
    resource_packs_stack_hook.expected[0] = 0xE92D47F0u;
    resource_packs_stack_hook.expected[1] = 0xE1A05000u;
    if (s->host.install_hook(&resource_packs_stack_hook)) return -84;
    return 0;
}
