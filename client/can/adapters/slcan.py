
import can
try:
    from termios import error as termios_error
except ImportError:
    # Dummy termios.error
    termios_error = Exception
import serial
from serial.serialutil import SerialException
from queue import Queue
import threading
import errno
from sys import exit


class SLCANInterface:
    """
    Implements support for CAN adapters which support the SLCAN API
    """

    MIN_MSG_LEN = len('t1230')

    def __init__(self, port):
        try:
            self.port = serial.Serial(port=port, timeout=1)
        except FileNotFoundError:
            # Port not found exception
            can.logging.critical("The specified port could not be found.")
            exit(1)
        except SerialException as e:
            if e.errno == errno.EACCES:
                # Permission denied
                can.logging.critical("You don't seem to have permission to open this port.")
                exit(e.errno)
            elif e.errno == errno.EBUSY:
                # Device or resource busy
                can.logging.critical("The specified port appears to be busy. Is it in use by another program?")
                exit(e.errno)
            else:
                # all other serial exceptions
                can.logging.critical("The specified port could not be opened.")
                raise
                exit(e.errno)
        except termios_error as e:
            if e.args[0] == 25:
                # Inappropriate ioctl for device
                can.logging.critical("The specified port appears to be busy. Is it in use by another program?")
                exit(e.args[0])
            else:
                # all other termios errors
                can.logging.critical("The specified port could not be opened.")
                raise
                exit(e.args[0])
        except:
            # all other exceptions
            can.logging.critical("The specified port could not be opened.")
            raise
            exit(1)

        # Queue for received CAN frames
        self.rx_queue = Queue()

        # Start independent thread for frame reception
        t = threading.Thread(target=self.spin)
        t.daemon = True
        t.start()

        # Configure adapter
        self.send_command('S8');    # bitrate 1Mbit
        self.send_command('O');    # open device
        self.port.reset_input_buffer()

    def spin(self):
        part = ''
        while True:
            try:
                part += self.port.read(1).decode('ascii')
            except serial.serialutil.SerialException:
                # Continue reading anyway until program terminates
                pass

            if part.startswith('\r'):
                part.lstrip('\r')

            if '\r' not in part:
                continue

            data = part.split('\r')
            data, part = data[:-1], data[-1]

            for frame in data:
                if frame is None:
                    continue
                frame = self.decode_frame(frame)
                if frame:
                    self.rx_queue.put(frame)
                    can.logging.debug("Received CAN frame via SLCAN adapter.")

    def send_command(self, cmd):
        cmd += '\r'
        cmd = cmd.encode('ascii')
        self.port.flushOutput()
        self.port.write(cmd)
        self.port.flushOutput()

    def decode_frame(self, msg):
        if len(msg) < self.MIN_MSG_LEN:
            return None

        cmd, msg = msg[0], msg[1:]
        if cmd == 'T':
            extended = True
            id_len = 8
        elif cmd == 't':
            extended = False
            id_len = 3
        else:
            return None

        if len(msg) < id_len + 1:
            return None
        can_id = int(msg[0:id_len], 16)
        msg = msg[id_len:]
        data_len = int(msg[0])
        msg = msg[1:]
        if len(msg) < 2 * data_len:
            return None
        data = [int(msg[i:i + 2], 16) for i in range(0, 2 * data_len, 2)]

        return can.Frame(id=can_id, data=bytearray(data), data_length=data_len, extended=extended)

    def encode_frame(self, frame):
        if frame.extended:
            cmd = 'T'
            can_id = '{:08x}'.format(frame.id)
        else:
            cmd = 't'
            can_id = '{:03x}'.format(frame.id)

        length = '{:x}'.format(frame.data_length)

        data = ''
        for b in frame.data:
            data += '{:02x}'.format(b)
        return cmd + can_id + length + data

    def send_frame(self, frame):
        can.logging.debug("Transmitting CAN frame via SLCAN adapter...")
        cmd = self.encode_frame(frame)
        self.send_command(cmd)

    def receive_frame(self):
        try:
            # Try to retrieve a frame from the queue within specified timeout period
            return self.rx_queue.get(block=True, timeout=3)
        except:
            return None
