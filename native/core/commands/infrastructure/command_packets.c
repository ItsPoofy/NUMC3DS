#include "../command_packets.h"
#include "../../internal.h"

typedef void (*PacketFactoryFn)(void **,unsigned);
typedef unsigned (*HashCodeFn)(const char *);

enum {
    PLAY_SOUND_PACKET_ID=0x57,STOP_SOUND_PACKET_ID=0x58,SET_TITLE_PACKET_ID=0x59,
    PLAY_SOUND_PACKET_HASH_OFFSET=0x08,PLAY_SOUND_PACKET_X_OFFSET=0x0c,PLAY_SOUND_PACKET_Y_OFFSET=0x10,PLAY_SOUND_PACKET_Z_OFFSET=0x14,
    PLAY_SOUND_PACKET_VOLUME_OFFSET=0x18,PLAY_SOUND_PACKET_PITCH_OFFSET=0x1c,STOP_SOUND_PACKET_HASH_OFFSET=0x08,STOP_SOUND_PACKET_ALL_OFFSET=0x0c,
    SET_TITLE_PACKET_ACTION_OFFSET=0x08,SET_TITLE_PACKET_TEXT_OFFSET=0x0c,SET_TITLE_PACKET_FADE_IN_OFFSET=0x10,SET_TITLE_PACKET_STAY_OFFSET=0x14,SET_TITLE_PACKET_FADE_OUT_OFFSET=0x18
};

int command_packet_stop_sound(void *player,const char *sound){
    void *packet=0,*sender;const char *name;unsigned hash=0;int all=!sound||!*sound;if(!player)return 0;
    sender=*(void **)((u8 *)player+SEAM_Entity_packetSenderOffset);if(!sender)return 0;if(!all){name=strip_ns(sound);hash=((HashCodeFn)SEAM_StringUtils_hashCode)(name);}
    ((PacketFactoryFn)SEAM_MinecraftPackets_createPacket)(&packet,STOP_SOUND_PACKET_ID);if(!packet)return 0;*(u32 *)((u8 *)packet+STOP_SOUND_PACKET_HASH_OFFSET)=hash;*(u8 *)((u8 *)packet+STOP_SOUND_PACKET_ALL_OFFSET)=(u8)(all?1:0);
    ((LoopbackSend)SEAM_LoopbackSend)(sender,packet);((PacketDtor)SEAM_StopSoundPacket_dtor)(packet);return 1;
}

int command_packet_play_sound(void *player,const char *sound,const float position[3],float volume,float pitch){
    void *packet=0,*sender;unsigned hash;if(!player||!sound||!position)return 0;sender=*(void **)((u8 *)player+SEAM_Entity_packetSenderOffset);if(!sender)return 0;
    hash=((HashCodeFn)SEAM_StringUtils_hashCode)(strip_ns(sound));((PacketFactoryFn)SEAM_MinecraftPackets_createPacket)(&packet,PLAY_SOUND_PACKET_ID);if(!packet)return 0;
    *(u32 *)((u8 *)packet+PLAY_SOUND_PACKET_HASH_OFFSET)=hash;*(int *)((u8 *)packet+PLAY_SOUND_PACKET_X_OFFSET)=(int)(position[0]*8.0f);*(int *)((u8 *)packet+PLAY_SOUND_PACKET_Y_OFFSET)=(int)(position[1]*8.0f);*(int *)((u8 *)packet+PLAY_SOUND_PACKET_Z_OFFSET)=(int)(position[2]*8.0f);*(float *)((u8 *)packet+PLAY_SOUND_PACKET_VOLUME_OFFSET)=volume;*(float *)((u8 *)packet+PLAY_SOUND_PACKET_PITCH_OFFSET)=pitch;
    ((LoopbackSend)SEAM_LoopbackSend)(sender,packet);((PacketDtor)SEAM_PlaySoundPacket_dtor)(packet);return 1;
}

int command_packet_set_title(void *player,const char *operation,const char *text,int fade_in,int stay,int fade_out){
    void *packet=0,*sender;int action;u32 title=0;if(!player||!operation)return 0;
    if(streq(operation,"clear"))action=0;else if(streq(operation,"reset"))action=1;else if(streq(operation,"title"))action=2;else if(streq(operation,"subtitle"))action=3;else if(streq(operation,"actionbar"))action=4;else if(streq(operation,"times"))action=5;else return 0;
    sender=*(void **)((u8 *)player+SEAM_Entity_packetSenderOffset);if(!sender)return 0;((PacketFactoryFn)SEAM_MinecraftPackets_createPacket)(&packet,SET_TITLE_PACKET_ID);if(!packet)return 0;
    *(int *)((u8 *)packet+SET_TITLE_PACKET_ACTION_OFFSET)=action;if(text){make_str(&title,text);((StrAssign)SEAM_StrAssign)((u8 *)packet+SET_TITLE_PACKET_TEXT_OFFSET,&title);drop_str(&title);}
    *(int *)((u8 *)packet+SET_TITLE_PACKET_FADE_IN_OFFSET)=fade_in;*(int *)((u8 *)packet+SET_TITLE_PACKET_STAY_OFFSET)=stay;*(int *)((u8 *)packet+SET_TITLE_PACKET_FADE_OUT_OFFSET)=fade_out;
    ((LoopbackSend)SEAM_LoopbackSend)(sender,packet);((PacketDtor)SEAM_SetTitlePacket_dtor)(packet);return 1;
}

int command_packet_broadcast_time(void *level,int time){
    u32 packet[3];void *sender;
    if(!level)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender||!*(void ***)sender)return 0;
    packet[0]=SEAM_SetTimePacket_vtable;packet[1]=0x00000102u;packet[2]=(u32)time;
    ((void (*)(void *,const void *))(*(void ***)sender)[SEAM_LoopbackPacketSender_sendVtableOffset/sizeof(void *)])(sender,packet);
    return 1;
}

int command_packet_broadcast_world_spawn(void *source,const int position[3]){
    u32 packet[8];void *dimension;void **vtable;
    if(!source||!position)return 0;
    dimension=((void *(*)(void *))SEAM_BlockSource_getDimension)(source);
    vtable=dimension?*(void ***)dimension:0;
    if(!vtable||!vtable[SEAM_Dimension_broadcastPacketVtableOffset/sizeof(void *)])return 0;
    zero(packet,sizeof(packet));packet[0]=SEAM_SetSpawnPositionPacket_vtable;packet[1]=0x00000102u;
    packet[2]=(u32)position[0];packet[3]=(u32)position[1];packet[4]=(u32)position[2];packet[5]=1;
    ((void (*)(void *,const void *,int))vtable[SEAM_Dimension_broadcastPacketVtableOffset/sizeof(void *)])(dimension,packet,0);
    return 1;
}

int command_packet_broadcast_difficulty(void *level,int difficulty){
    u32 packet[3];void *sender;
    if(!level||difficulty<0||difficulty>3)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender||!*(void ***)sender)return 0;
    packet[0]=0x009BD390u;
    packet[1]=0x00000102u;
    packet[2]=(u32)difficulty;
    ((void (*)(void *,const void *))(*(void ***)sender)[SEAM_LoopbackPacketSender_sendVtableOffset/sizeof(void *)])(sender,packet);
    return 1;
}

int command_packet_broadcast_gamerule_bool(void *level,const char *name,int value){
    u32 packet[6];u32 named_rule[3];u32 str_obj=0,scratch=0;void *sender;
    if(!level||!name)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender||!*(void ***)sender)return 0;
    ((StrCtor)SEAM_StrCtor)(&str_obj,name,&scratch);
    if(!str_obj)return 0;
    named_rule[0]=str_obj;
    named_rule[1]=0x00010000u;
    named_rule[2]=(u32)(value?1:0);
    packet[0]=0x009C247Cu;
    packet[1]=0x00000102u;
    packet[2]=(u32)&named_rule[0];
    packet[3]=(u32)&named_rule[3];
    packet[4]=(u32)&named_rule[3];
    packet[5]=0;
    ((void (*)(void *,const void *))(*(void ***)sender)[SEAM_LoopbackPacketSender_sendVtableOffset/sizeof(void *)])(sender,packet);
    ((StrDtor)SEAM_StrDtor)(&str_obj);
    return 1;
}

int command_packet_broadcast_gamerule_int(void *level,const char *name,int value){
    u32 packet[6];u32 named_rule[3];u32 str_obj=0,scratch=0;void *sender;
    if(!level||!name)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender||!*(void ***)sender)return 0;
    ((StrCtor)SEAM_StrCtor)(&str_obj,name,&scratch);
    if(!str_obj)return 0;
    named_rule[0]=str_obj;
    named_rule[1]=0x00020000u;
    named_rule[2]=(u32)value;
    packet[0]=0x009C247Cu;
    packet[1]=0x00000102u;
    packet[2]=(u32)&named_rule[0];
    packet[3]=(u32)&named_rule[3];
    packet[4]=(u32)&named_rule[3];
    packet[5]=0;
    ((void (*)(void *,const void *))(*(void ***)sender)[SEAM_LoopbackPacketSender_sendVtableOffset/sizeof(void *)])(sender,packet);
    ((StrDtor)SEAM_StrDtor)(&str_obj);
    return 1;
}
