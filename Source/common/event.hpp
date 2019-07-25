#ifndef KT_EVENT_HPP
#define KT_EVENT_HPP

#include <functional>

typedef int ktEventProcHandle_t;

const int KT_EVENT_PAYLOAD_SIZE = 256;

enum ktEventId {
    
};

struct ktEvent {
    int event;
    char payload[KT_EVENT_PAYLOAD_SIZE];
};

typedef std::function<void(ktEvent*)> ktEventProc_t;

ktEventProcHandle_t ktRegisterEventProc(ktEventProc_t proc);

void ktPostEvent(ktEvent e);
void ktPollEvents();

#endif
