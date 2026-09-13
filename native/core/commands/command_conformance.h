#ifndef NUMC3DS_COMMAND_CONFORMANCE_H
#define NUMC3DS_COMMAND_CONFORMANCE_H

void command_conformance_capture_begin(void);
void command_conformance_capture_end(void);
void command_conformance_capture_result(const char *message);
unsigned command_conformance_capture_count(void);
const char *command_conformance_capture_message(unsigned index);

#endif
