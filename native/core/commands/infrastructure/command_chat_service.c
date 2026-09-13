#include "../command_chat_service.h"
#include "../command_localization.h"
#include "../native_property_bag.h"
#include "../native_types.h"
#include "../../internal.h"

typedef void (*PacketBroadcastFn)(void *,const void *);

typedef struct {
    char *out;
    unsigned *length;
    char previous[MAX_TEXT+1];
    unsigned count;
} ChatTargetExpansion;

static int append_target_name(void *user,void *entity){
    ChatTargetExpansion *expansion=(ChatTargetExpansion *)user;char name[MAX_TEXT+1];
    if(!expansion||!entity)return 1;
    entity_name(entity,name,sizeof(name));
    if(expansion->count){
        if(expansion->count>1)append(expansion->out,expansion->length,", ");
        append(expansion->out,expansion->length,expansion->previous);
    }
    copy_text(expansion->previous,name,MAX_TEXT);
    expansion->count++;
    return 1;
}

static void append_expanded_target(void *origin,void *bag,const char *token,char *out,unsigned *length){
    ChatTargetExpansion expansion;
    zero(&expansion,sizeof(expansion));expansion.out=out;expansion.length=length;
    native_bag_set_string(bag,"_sayTarget",token);
    native_bag_visit_targets(origin,bag,"_sayTarget",append_target_name,&expansion);
    if(!expansion.count){append(out,length,token);return;}
    if(expansion.count>1)append(out,length," and ");
    append(out,length,expansion.previous);
}

static int send_text_packet(void *level,int type,const char *source,const char *message,int broadcast){
    NativeGstdString native_source,native_message;NativeVector parameters;u32 packet[7],scratch=0;void *sender;
    if(!level||!source||!message)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender)return 0;
    zero(&native_source,sizeof(native_source));zero(&native_message,sizeof(native_message));zero(&parameters,sizeof(parameters));zero(packet,sizeof(packet));
    ((StrCtor)SEAM_StrCtor)(&native_source,source,&scratch);((StrCtor)SEAM_StrCtor)(&native_message,message,&scratch);
    if(!native_source.handle||!native_message.handle){if(native_message.handle)((StrDtor)SEAM_StrDtor)(&native_message);if(native_source.handle)((StrDtor)SEAM_StrDtor)(&native_source);return 0;}
    ((PacketCtor)SEAM_TextPacket_ctor)(packet,type,&native_source,&native_message,&parameters);
    if(broadcast)((PacketBroadcastFn)SEAM_LoopbackSendBroadcast)(sender,packet);else ((LoopbackSend)SEAM_LoopbackSend)(sender,packet);
    ((PacketDtor)SEAM_TextPacket_dtor)(packet);((StrDtor)SEAM_StrDtor)(&native_message);((StrDtor)SEAM_StrDtor)(&native_source);
    return 1;
}

int command_chat_announcement(void *level,const char *sender,const char *message){
    const char *arguments[2];char rendered[MAX_TEXT+1];unsigned len=0;
    arguments[0]=sender;arguments[1]=message;
    if(!command_localize(rendered,sizeof(rendered),"chat.type.announcement",arguments,2)){
        rendered[0]=0;append(rendered,&len,"[");append(rendered,&len,sender?sender:"Server");append(rendered,&len,"] ");append(rendered,&len,message?message:"");
    }
    return send_text_packet(level,7,sender,rendered,1);
}

int command_chat_emote(void *level,const char *sender,const char *action){
    const char *arguments[2];char rendered[MAX_TEXT+1];unsigned len=0;
    arguments[0]=sender;arguments[1]=action;
    if(!command_localize(rendered,sizeof(rendered),"chat.type.emote",arguments,2)){
        rendered[0]=0;append(rendered,&len,"* ");append(rendered,&len,sender?sender:"Server");append(rendered,&len," ");append(rendered,&len,action?action:"");
    }
    return send_text_packet(level,1,"",rendered,1);
}

int command_chat_expand_target_names(void *origin,void *bag,const char *message,char *out,unsigned capacity){
    const char *cursor;unsigned length=0,token_length;char token[MAX_TEXT+1];
    if(!origin||!bag||!message||!out||capacity<MAX_TEXT+1)return 0;
    out[0]=0;cursor=message;
    while(*cursor){
        while(*cursor==' ')cursor++;
        if(!*cursor)break;
        token_length=0;
        while(cursor[token_length]&&cursor[token_length]!=' '&&token_length<MAX_TEXT)token[token_length]=cursor[token_length],token_length++;
        token[token_length]=0;cursor+=token_length;
        while(*cursor&&*cursor!=' ')cursor++;
        if(length)append(out,&length," ");
        if(token[0]=='@')append_expanded_target(origin,bag,token,out,&length);else append(out,&length,token);
    }
    return 1;
}
