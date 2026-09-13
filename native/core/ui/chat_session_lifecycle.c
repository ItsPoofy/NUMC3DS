#include "chat_session_lifecycle.h"
#include "chat_ui.h"
#include "../internal.h"
#include "../skin/remote_skin_service.h"
#include "../world/extended_chunk_storage.h"

#include "../../diagnostics/network_debug.h"

typedef void (*MinecraftGameLeaveGameFn)(void*,int);

static void on_leave_game(void*game,int exit_session){
    net_log_open(NET_LOG_INFO, "game", "leave_game");
    net_log_hex("game", (u32)game);
    net_log_dec("exit_session", (u32)exit_session);
    net_log_hex("caller", (u32)__builtin_return_address(0));
    net_log_close();
    ((MinecraftGameLeaveGameFn)s->chat_session_end.trampoline)(game,exit_session);
    chat_ui_flush_session();
    remote_skin_service_reset();
    extended_chunk_storage_reset();
}


int chat_session_lifecycle_install_hook(void){
    s->chat_session_end.target=SEAM_MinecraftGame_leaveGame;
    s->chat_session_end.replacement=(u32)on_leave_game;
    s->chat_session_end.expected[0]=0xE92D40F0u;
    s->chat_session_end.expected[1]=0xE24DD014u;
    return s->host.install_hook(&s->chat_session_end)?-35:0;
}
