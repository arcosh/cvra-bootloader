
#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stdbool.h>

#include <platform.h>

#ifndef CAN_FRAMES_BUFFERED
#define CAN_FRAMES_BUFFERED     300
#endif

/**
 * Struct containing all information about a CAN frame
 * which is relevant and therefore worth buffering
 */
typedef struct {
    uint32_t    id;
    uint8_t     dlc;
    uint8_t     data[8];
} can_frame_t;


/**
 * A FIFO for CAN frames
 */
typedef struct {
    can_frame_t buffer[CAN_FRAMES_BUFFERED];

    /**
     * Index of the position in which to store the next frame
     */
    volatile uint16_t push_index;

    /**
     * Index of the next frame to pop from the buffer
     */
    volatile uint16_t pop_index;
} fifo_t;


/**
 * Initialize empty FIFO buffer for CAN frames
 */
void fifo_init(fifo_t* fifo);

/**
 * Determine whether there is empty space in the buffer available or not
 */
bool fifo_is_full(fifo_t* fifo);

/**
 * Determine, whether a new frame is available or not
 */
bool fifo_is_empty(fifo_t* fifo);

/**
 * Retrieve one frame from the buffer
 */
void fifo_get_oldest_entry(
        fifo_t* fifo,
        uint32_t* id,
        uint8_t* dlc,
        uint8_t* data
        );

/**
 * Delete the oldest frame in the buffer
 */
void fifo_drop_oldest_entry(fifo_t* fifo);

#endif
