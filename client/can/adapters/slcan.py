
import can
import serial
from queue import Queue
import threading


class SLCANInterface:
    """
    Implements support for CAN adapters which support the SLCAN API
    """

    MIN_MSG_LEN = len('t1230')

    def __init__(self, port):
        self.port = serial.Serial(port=port, timeout=0.1)

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

    def send_command(self, cmd):
        cmd += '\r'
        cmd = cmd.encode('ascii')
        self.port.write(cmd)

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
        cmd = self.encode_frame(frame)
        self.send_command(cmd)

    def receive_frame(self):
        try:
            return self.rx_queue.get(True, 1)    # block with timeout 1 sec
        except:
            return None
