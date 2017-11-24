
import can
from .PCAN_Basic.Include.PCANBasic import *


class PeakPCANInterface:
    """
    Implements support for the Peak-System PCAN-USB adapter
    """

    def __init__(self):
        self.pcan = PCANBasic()
        self.channel = PCAN_USBBUS1
        baudrate = PCAN_BAUD_1M
        result = self.pcan.Initialize(self.channel, baudrate)
        if result != PCAN_ERROR_OK:
            can.logging.error("Failed to initialize PCAN adapter.")
        # result = self.pcan.FilterMessages(self.channel, 0x000, 0x123, PCAN_MODE_STANDARD)
        # if result != PCAN_ERROR_OK:
        #    can.logging.warning("Failed to configure frame filtering on PCAN adapter.")

    def send_frame(self, frame):
        can.logging.debug("Transmitting CAN frame via PCAN adapter...")
        msg = TPCANMsg()
        msg.ID = frame.id
        if frame.extended:
            msg.MSGTYPE = PCAN_MESSAGE_EXTENDED
        else:
            msg.MSGTYPE = PCAN_MESSAGE_STANDARD
        msg.LEN = frame.data_length
        for i in range(msg.LEN):
            msg.DATA[i] = int(frame.data[i])
        self.pcan.Write(self.channel, msg)

    def receive_frame(self):
        result = self.pcan.Read(self.channel)
        if result[0] != PCAN_ERROR_OK:
            return None

        # Get TPCANMsg
        msg = result[1]

        if msg.MSGTYPE == 0x80:    # PCAN_MESSAGE_STATUS
            return None

        if msg.MSGTYPE in [0x00, 0x01, 0x02]:    # [PCAN_MESSAGE_STANDARD, PCAN_MESSAGE_RTR, PCAN_MESSAGE_EXTENDED]
            can.logging.debug("Received CAN frame via PCAN adapter.")
            return can.Frame(id=msg.ID, data=bytearray(msg.DATA), data_length=msg.LEN, extended=(msg.MSGTYPE == PCAN_MESSAGE_EXTENDED))

        return None
