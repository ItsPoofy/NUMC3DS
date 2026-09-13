#include "network_module.h"
#include "../../diagnostics/network_debug.h"
#include "protocol/mcpe_protocol_override.h"
#include "protocol/mcpe_resource_pack_compat.h"
#include "protocol/mcpe_start_game_compat.h"
#include "protocol/mcpe_crafting_data_compat.h"
#include "protocol/mcpe_chunk_compat.h"
#include "protocol/mcpe_item_compat.h"
#include "protocol/mcpe_map_compat.h"
#include "raknet/stock_socket_adapter.h"
#include "session/mcpe_transport_selector.h"
#ifdef NUMC3DS_NETWORK_DIAGNOSTICS
#include "../../diagnostics/mcpe_connect_probe.h"
#endif
#include "peers/mcpe_peer_activation.h"
#include "peers/mcpe_handshake_activation.h"
#include "discovery/mcpe_discovery_ui.h"

static int network_install_step(const char *name, int result)
{
    net_log_open(NET_LOG_INFO, "install", name);
    net_log_signed("result", result);
    net_log_close();
    return result;
}

int network_module_install(void)
{
    int result;
    net_log_set_level(NET_LOG_INFO);
    net_log_open(NET_LOG_INFO, "install", "begin");
    net_log_dec("level", (u32)net_log_level());
    net_log_close();
    result = network_install_step("protocol_override", mcpe_protocol_override_install());
    if (result) return result;
    result = network_install_step("resource_pack_compat", mcpe_resource_pack_compat_install());
    if (result) return result;
    result = network_install_step("start_game_compat", mcpe_start_game_compat_install());
    if (result) return result;
    result = network_install_step("crafting_data_compat", mcpe_crafting_data_compat_install());
    if (result) return result;
    result = network_install_step("chunk_compat", mcpe_chunk_compat_install());
    if (result) return result;
    result = network_install_step("item_compat", mcpe_item_compat_install());
    if (result) return result;
    result = network_install_step("map_compat", mcpe_map_compat_install());
    if (result) return result;

    result = network_install_step("stock_socket_adapter", mcpe_stock_socket_adapter_install());
    if (result) return result;
    result = network_install_step("transport_selector", mcpe_transport_selector_install());
    if (result) return result;
#ifdef NUMC3DS_NETWORK_DIAGNOSTICS
    result = network_install_step("connect_probe", mcpe_connect_probe_install());
    if (result) return result;
#endif
    result = network_install_step("peer_activation", mcpe_peer_activation_install());
    if (result) return result;
    result = network_install_step("handshake_activation", mcpe_handshake_activation_install());
    if (result) return result;
    result = network_install_step("discovery_ui", mcpe_discovery_ui_install());
    if (result) return result;
    net_log_open(NET_LOG_INFO, "install", "complete");
    net_log_close();
    return 0;
}
