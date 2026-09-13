#ifndef NUMC3DS_COMMAND_CHAT_SERVICE_H
#define NUMC3DS_COMMAND_CHAT_SERVICE_H

int command_chat_announcement(void *level,const char *sender,const char *message);
int command_chat_emote(void *level,const char *sender,const char *action);
int command_chat_expand_target_names(void *origin,void *bag,const char *message,char *out,unsigned capacity);

#endif
