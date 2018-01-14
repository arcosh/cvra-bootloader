
#include "can_fifo.h"


void fifo_init(fifo_t* fifo) {
    fifo->push_index = 0;
    fifo->pop_index = 0;
}


bool fifo_is_full(fifo_t* fifo) {
    // The buffer is full, when the next push would increment the push_index to the pop_index.
    return ((fifo->push_index + 1) % CAN_FRAMES_BUFFERED == fifo->pop_index);
}


bool fifo_is_empty(fifo_t* fifo) {
    // When the push_index is shifted against the pop_index, data is available for popping.
    return (fifo->pop_index == fifo->push_index);
}



void fifo_get_oldest_entry(
        fifo_t* fifo,
        uint32_t* id,
        uint8_t* dlc,
        uint8_t* data
        ) {

    // Cache the current pop_index
    uint16_t index = fifo->pop_index;

    // Copy frame from buffer
    *id = fifo->buffer[index].id;
    *dlc = fifo->buffer[index].dlc;
    for (uint8_t i=0; i<8; i++) {
        data[i] = fifo->buffer[index].data[i];
    }
}


void fifo_drop_oldest_entry(fifo_t* fifo) {
    // It is not necessary to actually delete the old data, since it will be overwritten by the next push anyway.
    fifo->pop_index = (fifo->pop_index + 1) % CAN_FRAMES_BUFFERED;
}
