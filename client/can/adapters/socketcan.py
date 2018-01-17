
import can
import socket
import struct
from queue import Queue
import threading
import errno
from sys import exit


class SocketCANInterface:
    """
    Implements support for CAN adapters which provide a SocketCAN interface
    """

    # See <linux/can.h> for format
    CAN_FRAME_FMT = "=IB3x8s"
    CAN_FRAME_SIZE = struct.calcsize(CAN_FRAME_FMT)

    def __init__(self, interface):
        """
        Initiates a CAN connection on the given interface (e.g. 'can0').
        """

        try:
            from socket import AF_CAN
        except:
            # AF_CAN is not available on Windows.
            can.logging.critical("It appears, your machine doesn't support SocketCAN.")
            exit(1)

        # Creates a raw CAN connection and binds it to the given interface.
        self.socket = socket.socket(socket.AF_CAN,
                                    socket.SOCK_RAW,
                                    socket.CAN_RAW)

        try:
            self.socket.bind((interface,))
        except OSError as e:
            if (e.errno == errno.ENOENT) or (e.errno == errno.ENXIO) or (e.errno == errno.ENODEV):
                # No such device
                can.logging.critical("The specified interface could not be found.")
                exit(1)
            elif e.errno == errno.ENETDOWN:
                # Network is down
                can.logging.critical("The CAN network is down.")
                exit(errno.ENETDOWN)
            else:
                # all other OSError exceptions
                can.logging.critical("Unable to bind to the specified interface.")
                raise
                exit(1)
        except Exception as e:
            # all other exceptions
            can.logging.critical("Unable to bind to the specified interface.")
            raise
            exit(1)

        # Set transmission and reception timeout
        self.socket.settimeout(2.)

        # Set buffer sizes
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4096)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)

        # Create queue for received CAN frames
        self.rx_queue = Queue()

        # Start independent thread for frame reception
        t = threading.Thread(target=self.loop)
        t.daemon = True
        t.start()


    def loop(self):
        while True:
            try:
                frame, _ = self.socket.recvfrom(self.CAN_FRAME_SIZE)
            except socket.timeout:
                continue

            can_id, can_dlc, data = struct.unpack(self.CAN_FRAME_FMT, frame)
            frame = can.Frame(id=can_id, data=data[:can_dlc])
            self.rx_queue.put(frame)
            can.logging.debug("Received CAN frame from socket.")


    def send_frame(self, frame):
        can.logging.debug("Transmitting CAN frame via socket...")
        data = frame.data.ljust(8, b'\x00')
        data = struct.pack(self.CAN_FRAME_FMT,
                           frame.id,
                           len(frame.data),
                           data)

        try:
            self.socket.send(data)
        except socket.timeout:
            can.logging.warning("Socket transmission timed out.")
            pass
        except OSError as e:
            if e.errno == errno.ENOBUFS:
                # Buffer overflow
                can.logging.critical("Transmission buffer overflow. Probably CAN frames are not being acknowledged properly on the bus.")
                exit(errno.ENOBUFS)
            elif e.errno == errno.ENETDOWN:
                # Network is down
                can.logging.critical("The CAN network is down.")
                exit(errno.ENETDOWN)
            else:
                raise
                exit(1)
        except:
            raise
            exit(1)


    def receive_frame(self):
        try:
            # Try to retrieve a frame from the queue within specified timeout period
            return self.rx_queue.get(block=True, timeout=3)
        except:
            return None
