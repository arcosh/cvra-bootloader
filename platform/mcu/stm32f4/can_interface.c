
/**
 * TODO: In later versions of libopencm3 it might be sufficient
 * (and more appropriate) to include <libopencm3/stm32/nvic.h>
 */
#include <libopencm3/stm32/f4/nvic.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>
#include <can_interface.h>
#include <can_datagram.h>

/*
 * Include platform-specific configurations (platform.h)
 * in order to select, which CAN peripheral should be used
 */
#include <platform.h>


#ifdef CAN_RX_BUFFER_ENABLED
#include <can_fifo.h>

fifo_t can_rx_fifo;
#endif


void can_interface_init()
{
    /*
     * Enable clock on required GPIO ports
     */
    #if (GPIO_PORT_CAN_TX == GPIOA) || (GPIO_PORT_CAN_RX == GPIOA) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOA)
    rcc_periph_clock_enable(RCC_GPIOA);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOB) || (GPIO_PORT_CAN_RX == GPIOB) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOB)
    rcc_periph_clock_enable(RCC_GPIOB);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOC) || (GPIO_PORT_CAN_RX == GPIOC) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOC)
    rcc_periph_clock_enable(RCC_GPIOC);
    #endif
    #if (GPIO_PORT_CAN_TX == GPIOD) || (GPIO_PORT_CAN_RX == GPIOD) || (defined(USE_CAN_ENABLE) && GPIO_PORT_CAN_ENABLE == GPIOD)
    rcc_periph_clock_enable(RCC_GPIOD);
    #endif

    /*
     * In microcontrollers with dual-bxCAN (master+slave) configurations
     * it is obligatory to enable the master CAN's clock even if the master CAN isn't used,
     * because the master is managing the slave's access to the dual-bxCAN's SRAM.
     */
    rcc_periph_clock_enable(RCC_CAN1);
    #ifdef USE_CAN2
    rcc_periph_clock_enable(RCC_CAN2);
    #endif

    // Setup CAN pins
    gpio_mode_setup(
            GPIO_PORT_CAN_TX,
            GPIO_MODE_AF,
            GPIO_PUPD_PULLUP,
            GPIO_PIN_CAN_TX
            );
    gpio_set_af(
            GPIO_PORT_CAN_TX,
            GPIO_AF_CAN,
            GPIO_PIN_CAN_TX
            );

    gpio_mode_setup(
            GPIO_PORT_CAN_RX,
            GPIO_MODE_AF,
            GPIO_PUPD_PULLUP,
            GPIO_PIN_CAN_RX
            );
    gpio_set_af(
            GPIO_PORT_CAN_RX,
            GPIO_AF_CAN,
            GPIO_PIN_CAN_RX
            );

    #ifdef USE_CAN_ENABLE
    gpio_mode_setup(
            GPIO_PORT_CAN_ENABLE,
            GPIO_MODE_OUTPUT,
            GPIO_PUPD_NONE,
            GPIO_PIN_CAN_ENABLE
            );
    gpio_set_output_options(
            GPIO_PORT_CAN_ENABLE,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_2MHZ,
            GPIO_PIN_CAN_ENABLE
            );
    #ifndef CAN_ENABLE_INVERTED
    gpio_set(
            GPIO_PORT_CAN_ENABLE,
            GPIO_PIN_CAN_ENABLE
            );
    #else
    gpio_clear(
            GPIO_PORT_CAN_ENABLE,
            GPIO_PIN_CAN_ENABLE
            );
    #endif // CAN_ENABLE_INVERTED
    #endif // USE_CAN_ENABLE

    /*
     * CAN1 and CAN2 are running from APB1 clock (18MHz).
     * Therefore 500kbps can be achieved with prescaler=2 like this:
     * 18MHz / 2 = 9MHz
     * 9MHz / (1tq + 10tq + 7tq) = 500kHz => 500kbit
     */
    can_init(CAN,             // Interface
             false,           // Time triggered communication mode (TTCM)
             true,            // Automatic bus-off management (ABOM)
             true,            // Automatic wakeup mode (AWUM)
             false,           // No automatic retransmission (NART)
             false,           // Receive FIFO locked mode.
             false,           // Transmit FIFO priority.
             CAN_BTR_SJW_1TQ, // Resynchronization time quanta jump width
//             CAN_BTR_TS1_10TQ,// Time segment 1 time quanta width
//             CAN_BTR_TS2_7TQ, // Time segment 2 time quanta width
             CAN_BTR_TS1_16TQ,
             CAN_BTR_TS2_1TQ,
             1,               // Prescaler
             false,           // Loopback
             false);          // Silent

    #ifdef CAN_RX_BUFFER_ENABLED
    fifo_init(&can_rx_fifo);
    #endif  // CAN_RX_BUFFER_ENABLED

    #ifdef CAN_USE_INTERRUPTS
    #ifdef USE_CAN1
    // Only enable FIFO0 message pending interrupt
    CAN_IER(CAN1) = CAN_IER_FMPIE0;
    nvic_set_priority(NVIC_CAN1_RX0_IRQ, 7);
    nvic_enable_irq(NVIC_CAN1_RX0_IRQ);
    #endif  // USE_CAN1

    #ifdef USE_CAN2
    // Only enable FIFO0 message pending interrupt
    CAN_IER(CAN2) = CAN_IER_FMPIE0;
    nvic_set_priority(NVIC_CAN2_RX0_IRQ, 7);
    nvic_enable_irq(NVIC_CAN2_RX0_IRQ);
    #endif  // USE_CAN2
    #endif  // CAN_USE_INTERRUPTS
}


void can_set_filters(uint32_t id)
{
    /*
     * A filter's bank number i.e. slave start bank number
     * defines, at which bank number the filter banks
     * should route incoming and matching frames to
     * slave (CAN2) FIFOs instead of master (CAN1) FIFOs.
     */
    #ifdef USE_CAN1
        // Filters should route to CAN1 FIFOs
        uint8_t bank_number = 27;
    #endif
    #ifdef USE_CAN2
        // Filters should route to CAN2 FIFOs
        uint8_t bank_number = 0;
    #endif

    /*
     * TODO:
     * Workaround for incorrect shift value in libopencm3
     * Fixed since libopencm3 commit 5fc4f4
     */
    #ifdef CAN_FMR_CAN2SB_SHIFT
    #undef CAN_FMR_CAN2SB_SHIFT
    #endif
    #define CAN_FMR_CAN2SB_SHIFT    8

    /*
     * Configure the start slave bank
     * by writing to the filter master register (CAN_FMR)
     */
    CAN_FMR(CAN1) &= ~((uint32_t) CAN_FMR_CAN2SB_MASK);
    CAN_FMR(CAN1) |= (uint32_t) (bank_number << CAN_FMR_CAN2SB_SHIFT);

    #ifndef CAN_FILTERS_ENABLED
    /*
     * Configure bxCAN filters for promiscious mode: Receive all frames on the bus
     * CAVEAT: On chatty buses this might overload the MCU's frame processing capabilities
     */
    can_filter_id_mask_32bit_init(
         CAN1,   // Must be CAN1, even when CAN2 is used, because CAN2 filters are managed by CAN1.
         0,      // filter nr
         0,      // id: only std id, no rtr
         6 | (7<<29), // mask: match only std id[10:8] = 0 (bootloader frames)
         0,      // assign to fifo0
         true    // enable
         );
    #else
    /*
     * Configure CAN filters to only receive broadcast datagram frames and
     * frames of datagrams addressed specifically to this device
     *
     * Read more about bxCAN filters in the STM32F446 RM Rev.3 p.1049:
     *  Figure 391. Filter bank scale configuration - register organization
     */
    can_filter_id_mask_32bit_init(
        CAN1,           // Must be CAN1, even when CAN2 is used, because CAN2 filters are managed by CAN1.
        1,              // Filter number
        0x080 << 21,    // Filter ID
        (0x7FF << 21) | 0x6,    // Filter mask: StdID=id, IDE=Std, RTR=0
        0,              // FIFO
        true            // Enable
        );
    can_filter_id_mask_32bit_init(
        CAN1,
        2,
        0x000 << 21,
        (0x7FF << 21) | 0x6,
        0,
        true
        );
    can_filter_id_mask_32bit_init(
        CAN1,
        3,
        (id | ID_START_MASK) << 21,
        (0x7FF << 21) | 0x6,
        0,
        true
        );
    can_filter_id_mask_32bit_init(
        CAN1,
        4,
        id << 21,
        (0x7FF << 21) | 0x6,
        0,
        true
        );
    #endif  // CAN_FILTERS_ENABLED
}


/**
 * Determine, whether one or more CAN frames are ready in the reception FIFO
 */
inline bool can_frame_received()
{
    // FMP[1:0]: Number of pending messages in the FIFO
    return ((CAN_RF0R(CAN) & CAN_RF0R_FMP0_MASK) > 0);
}


/**
 * Determine, whether the last CAN transmission requests completed
 */
inline bool can_transmission_complete()
{
    // Transmit status register: Transmit mailbox 0 request complete
    return ((CAN_TSR(CAN) & CAN_TSR_RQCP0) > 0);
}


/**
 * Determine, whether the last CAN transmission completed successfully
 */
inline bool can_transmission_successful()
{
    // See Reference Manual, Figure 389: Transmit mailbox states
    // RQCP=1 && TXOK=1: Transmit succeeded
    // RQCP=1 && TXOK=0: Transmit failed (only if automatic re-transmission is disabled: NART=true)
    return can_transmission_complete() && ((CAN_TSR(CAN) & CAN_TSR_TXOK0) > 0);
}


/**
 * Issue an abort signal to all three CAN transmit mailboxes
 */
inline void can_abort_all_transmissions()
{
    // TODO: CAN_TSR_ABRQ2 is erroneously named CAN_TSR_TABRQ2 in the used revision of libopencm3. This should be fixed later on.

    // Set the abort bits
    CAN_TSR(CAN) |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_TABRQ2;
    // Clear the abort bits, not to have them interfer with future transmission requests
    CAN_TSR(CAN) &= ~(CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_TABRQ2);
}


#ifdef CAN_RX_BUFFER_ENABLED
/**
 * Receive a frame from the CAN FIFO to the buffer
 */
inline void can_receive_to_buffer()
{
    // Exit if no frame was received
    if (!can_frame_received())
        return;

    // Exit if no buffer space is available
    if (fifo_is_full(&can_rx_fifo))
        return;

    // Discarded frame properties
    uint32_t filter_id;
    bool extended_id;
    bool rtr_bit;

    // Buffered frame properties
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];

    // Retrieve one frame from the CAN FIFO
    can_receive(
        CAN,
        0,
        true,
        &id,
        &extended_id,
        &rtr_bit,
        &filter_id,
        &dlc,
        data
    );

    // Don't buffer irrelevant frames
    // TODO: Use config.id below
//    if ((id != 0x080)
//     && (id != 0x000)
//     && (id != 0x081)
//     && (id != 0x001))
//        return;

    can_rx_fifo.buffer[can_rx_fifo.push_index].id = id;
    can_rx_fifo.buffer[can_rx_fifo.push_index].dlc = dlc;
    for (uint8_t i=0; i<8; i++)
        can_rx_fifo.buffer[can_rx_fifo.push_index].data[i] = data[i];
    can_rx_fifo.push_index = (can_rx_fifo.push_index + 1) % CAN_FRAMES_BUFFERED;
}

/**
 * Pop a CAN frame from the buffer
 */
bool can_receive_from_buffer(uint32_t* id, uint8_t* message, uint8_t* length)
{
    if (fifo_is_empty(&can_rx_fifo))
        return false;

    // Return oldest new frame
    fifo_get_oldest_entry(
            &can_rx_fifo,
            id,
            length,
            message
            );
    fifo_drop_oldest_entry(&can_rx_fifo);

    return true;
}
#endif  // CAN_RX_BUFFER_ENABLED


#ifdef CAN_USE_INTERRUPTS
/**
 * CAN RX FIFO0 interrupt request handler
 */
void can2_rx0_isr()
{
    can_receive_to_buffer();
}
#endif  // CAN_USE_INTERRUPTS


bool can_interface_read_message(uint32_t *id, uint8_t *message, uint8_t *length, uint32_t retries)
{
#ifdef CAN_RX_BUFFER_ENABLED

    if (!fifo_is_empty(&can_rx_fifo))
    {
        return can_receive_from_buffer(id, message, length);
    }
    return false;

#else

    uint32_t fid;
    uint8_t len;
    bool ext, rtr;

    while (retries-- != 0 && (!can_frame_received()));

    if (!can_frame_received()) {
        return false;
    }

    can_receive(
        CAN,        // canport
        0,          // fifo
        true,       // release
        id,         // can id
        &ext,       // extended id
        &rtr,       // transmission request
        &fid,       // filter id
        &len,       // length
        message
    );

    *length = len;

    return true;

#endif
}

bool can_interface_send_message(uint32_t id, uint8_t *message, uint8_t length, uint32_t retries)
{
    do {
        // Abort any ongoing transmissions
        can_abort_all_transmissions();

        // Push the frame to an available transmit mailbox and initiate transmission
        // unit8_t mailbox =
        can_transmit(
            CAN,        // CAN port
            id,         // CAN id
            false,      // Standard ID
            false,      // RTR bit not set
            length,     // Data length (DLC)
            message     // Pointer to data bytes
        );

        // Timeout: 100000 @ 36MHz = min. 2.8ms
        uint32_t timeout = 100000;
        while ((!can_transmission_complete()) && timeout-- > 0);
        if (timeout == 0) {
            // Transmission timed out
            can_abort_all_transmissions();
            continue;
        }

        if (can_transmission_successful()) {
            // Frame was successfully transmitted
            return true;
        }

    } while (retries-- > 0);

    return false;
}
