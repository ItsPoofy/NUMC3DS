#include "command_conformance.h"
#include "../rt.h"

enum { CONFORMANCE_CAPTURE_CAPACITY=16 };

static char captured[CONFORMANCE_CAPTURE_CAPACITY][MAX_TEXT+1];
static unsigned captured_count;
static int capture_active;

void command_conformance_capture_begin(void){
    unsigned index;
    capture_active=1;
    captured_count=0;
    for(index=0;index<CONFORMANCE_CAPTURE_CAPACITY;index++)captured[index][0]=0;
}

void command_conformance_capture_end(void){capture_active=0;}

void command_conformance_capture_result(const char *message){
    unsigned index;
    if(!capture_active)return;
    if(captured_count==CONFORMANCE_CAPTURE_CAPACITY){
        for(index=1;index<CONFORMANCE_CAPTURE_CAPACITY;index++)copy_text(captured[index-1],captured[index],MAX_TEXT);
        captured_count--;
    }
    copy_text(captured[captured_count++],message?message:"",MAX_TEXT);
}

unsigned command_conformance_capture_count(void){return captured_count;}

const char *command_conformance_capture_message(unsigned index){
    return index<captured_count?captured[index]:"";
}
