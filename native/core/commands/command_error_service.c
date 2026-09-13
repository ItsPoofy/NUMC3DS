#include "command_error_service.h"
#include "../util/string_util.h"
#include "../util/math_util.h"
#include "../rt.h"
#include "../state.h"
#include "native_output.h"
#include "command_conformance.h"
#include "command_localization.h"

const char command_sent[] = "NuMC3DS chat: command submitted\n";
const char command_unavailable[] = "Commands are not enabled in this world.";
const char command_failed[] = "Command execution failed.";
const char command_permission[] = "You do not have permission to use this command.";
const char command_syntax[] = "Syntax error. Use /help for command syntax.";
const char sent[] = "NuMC3DS chat: message sent\n";

void result_text(const char* t) {
    command_conformance_capture_result(t);
    if (s && s->exec_output_bag) {
        native_output_set_status(s->exec_output_bag, native_command_status_code(s->exec_result), t);
    }
}

void result_number(const char* prefix, int value) {
    char out[MAX_TEXT + 1];
    unsigned n = 0;
    out[0] = 0;
    append(out, &n, prefix);
    append_int(out, &n, value);
    result_text(out);
}

int fail_syntax(void) {
    if (s && s->exec_native_callback) s->exec_result = native_command_result_error();
    result_text(command_syntax);
    return 0;
}

int fail_command(const char* message) {
    if (s && s->exec_native_callback) s->exec_result = native_command_result_error();
    result_text(message);
    return 0;
}

int fail_command_localized(const char* key) {
    char text[MAX_TEXT + 1];
    if (command_localize(text, sizeof(text), key, 0, 0)) return fail_command(text);
    return fail_command(key);
}

int fail_command_localized_args(const char* key, const char* const* arguments, unsigned argument_count) {
    char text[MAX_TEXT + 1];
    if (command_localize(text, sizeof(text), key, arguments, argument_count)) return fail_command(text);
    return fail_command(key);
}

int fail_number_too_small(int value, int minimum) {
    char output[MAX_TEXT + 1];
    unsigned length = 0;
    output[0] = 0;
    append(output, &length, "The number you have entered (");
    append_int(output, &length, value);
    append(output, &length, ") is too small, it must be at least ");
    append_int(output, &length, minimum);
    return fail_command(output);
}

int fail_number_too_big(int value, int maximum) {
    char output[MAX_TEXT + 1];
    unsigned length = 0;
    output[0] = 0;
    append(output, &length, "The number you have entered (");
    append_int(output, &length, value);
    append(output, &length, ") is too big, it must be at most ");
    append_int(output, &length, maximum);
    return fail_command(output);
}

void item_not_found(const char* name) {
    char out[MAX_TEXT + 1];
    unsigned n = 0;
    out[0] = 0;
    append(out, &n, "There is no such item with name ");
    append(out, &n, name);
    result_text(out);
}

int send_command_chat(void* player, const char* source_text, const char* message) {
    StrCtor ctor = (StrCtor)0x002FF221u;
    StrDtor dtor = (StrDtor)0x002FEBBDu;
    PacketCtor packet_ctor = (PacketCtor)0x0016B52Cu;
    PacketDtor packet_dtor = (PacketDtor)0x0016B694u;
    LoopbackSend loopback_send = (LoopbackSend)0x003F4908u;
    numc3ds_u32 source = 0, msg = 0, source_scratch = 0, msg_scratch = 0, vec[3], pkt[8];
    void* sender = *(void**)((unsigned char*)player + 0x18ec);
    if (!sender) return 0;
    zero(vec, sizeof(vec));
    zero(pkt, sizeof(pkt));
    ctor(&source, source_text, &source_scratch);
    ctor(&msg, message, &msg_scratch);
    packet_ctor(pkt, 1, &source, &msg, vec);
    loopback_send(sender, pkt);
    packet_dtor(pkt);
    dtor(&msg);
    dtor(&source);
    return 1;
}

void action_message(void* player, const char* message) {
    CopyEntityName copy_name = (CopyEntityName)0x00724730u;
    StrDtor dtor = (StrDtor)0x002FEBBDu;
    numc3ds_u32 name = 0;
    char out[MAX_TEXT + 1];
    char* raw;
    unsigned n = 0;
    copy_name(&name, player);
    raw = *(char**)&name;
    out[0] = 0;
    append(out, &n, "* ");
    append(out, &n, raw && raw[0] ? raw : "Player");
    append(out, &n, " ");
    append(out, &n, message);
    dtor(&name);
    send_command_chat(player, "", out);
}
