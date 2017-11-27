class Frame:
    """
    A single CAN frame.
    """

    def __init__(self, id=0, data=None, extended=False,
                 transmission_request=False,
                 data_length=None):

        # If no data is provided, start with empty frame.
        if data is None:
            data = bytes()

        # Restrict length to 8 bytes
        if len(data) > 8:
            data = data[:8]

        self.id = id
        self.data = data

        # Extract data_length, (only) if not provided
        if data_length is None:
            self.data_length = len(self.data)
        else:
            self.data_length = data_length

        self.transmission_request = transmission_request
        self.extended = extended

    def __eq__(self, other):
        return self.id == other.id and self.data == other.data

    def __str__(self):
        if self.extended:
            frame = '{:08X}'.format(self.id)
        else:
            frame = '{:03X}'.format(self.id)
        frame += ' [{}]'.format(self.data_length)
        if not self.transmission_request:
            for b in self.data:
                frame += ' {:02X}'.format(int(b));
        return frame
