#ifndef NUMC3DS_COMMAND_PACKETS_H
#define NUMC3DS_COMMAND_PACKETS_H

int command_packet_stop_sound(void *player,const char *sound);
int command_packet_play_sound(void *player,const char *sound,const float position[3],float volume,float pitch);
int command_packet_set_title(void *player,const char *operation,const char *text,int fade_in,int stay,int fade_out);
int command_packet_broadcast_time(void *level,int time);
int command_packet_broadcast_world_spawn(void *source,const int position[3]);

#endif
