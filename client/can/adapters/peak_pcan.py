
import can
from queue import Queue
import threading
from sys import exit

try:
    from .PCAN_Basic.Include.PCANBasic import *
except ImportError:
    can.logging.critical("Unable to load PCANBasic library.")
    exit(1)




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

        # Create queue for received CAN frames
        self.rx_queue = Queue()

        # Start independent thread for frame reception
        self.t = threading.Thread(target=self.loop)
        self.t.daemon = True
        self.t.start()


    def loop(self):
        while True:
            result = self.pcan.Read(self.channel)

            # Nothing received
            if result[0] != PCAN_ERROR_OK:
                continue

            # Get TPCANMsg
            msg = result[1]

            # PCAN_MESSAGE_STATUS
            if msg.MSGTYPE == 0x80:
                continue

            # [PCAN_MESSAGE_STANDARD, PCAN_MESSAGE_RTR, PCAN_MESSAGE_EXTENDED]
            if msg.MSGTYPE in [0x00, 0x01, 0x02]:
                frame = can.Frame(id=msg.ID, data=bytearray(msg.DATA), data_length=msg.LEN, extended=(msg.MSGTYPE == PCAN_MESSAGE_EXTENDED))
                self.rx_queue.put(frame)
                can.logging.debug("Received CAN frame via PCAN adapter.")


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
        try:
            # Try to retrieve a frame from the queue within specified timeout period
            return self.rx_queue.get(block=True, timeout=3)
        except:
            return None
