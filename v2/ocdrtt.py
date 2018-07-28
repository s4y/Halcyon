#!/usr/bin/env python3
import socket
import struct
import sys
import time

class Session:
    def __init__(self, app_ram_start=0x20000000, addr=('127.0.0.1', 6666)):
        self.app_ram_start = app_ram_start
        self.addr = addr
        self.socket = None
        self.control_block_location = None
        self.buf_counts = None

    def __cmd(self, input):
        if not self.socket:
            self.socket = socket.create_connection(self.addr)
        try:
            self.socket.send(input.encode() + b'\x1a')

            res = self.socket.recv(4096)
            while res[-1] != 0x1a:
                res += self.socket.recv(4096)
            return res[:-1].decode()
        except socket.error:
            self.socket = None
            return _cmd(input)

    def __read_mem(self, addr, len):
        if not len:
            return bytes(0)
        r = self.__cmd('ocd_mdb {} {}'.format(hex(addr), hex(len)))
        return bytes(int(val, 16) for line in r.strip().split('\n') for val in line.split(': ')[1].strip().split(' '))

    def __write_word(self, addr, word):
        self.__cmd('ocd_mww {} {}'.format(hex(addr), hex(word)))

    def __get_control_block_location(self):
        if not self.control_block_location:
            breadcrumb = b'SEGGER RTT'
            chunk = self.__read_mem(self.app_ram_start, 16384)
            offset = chunk.find(breadcrumb)
            if offset == -1:
                raise Exception("Control block not found")
            self.control_block_location = self.app_ram_start + offset + 16
        return self.control_block_location

    def __read_control_block(self):
        addr = self.__get_control_block_location()
        ctrl_fmt = 'II'
        ctrl_len = struct.calcsize(ctrl_fmt)
        if not self.buf_counts:
            control_block = self.__read_mem(addr, ctrl_len)
            self.buf_counts = struct.unpack(ctrl_fmt, control_block)

        (n_up_bufs, n_down_bufs) = self.buf_counts
        buf_fmt = 'IIIIII'
        buf_len = struct.calcsize(buf_fmt)
        bufs_start = addr + struct.calcsize(ctrl_fmt)
        buf_blocks = self.__read_mem(bufs_start, (n_up_bufs + n_down_bufs) * buf_len)

        def unpack_buf(i):
            buf_offset = buf_len * i
            return (
                bufs_start + buf_offset,
                struct.unpack(buf_fmt, buf_blocks[buf_offset:buf_offset + buf_len])
            )
        return (
            [unpack_buf(i) for i in range(0, n_up_bufs)],
            [unpack_buf(i) for i in range(n_up_bufs, n_up_bufs + n_down_bufs)]
        )

    def read_all(self):
        def read_buf(addr, name, buffer, size, woffset, roffset, flags):
            if woffset == roffset:
                return b''
            self.__write_word(addr + struct.calcsize('IIII'), woffset)
            buf = self.__read_mem(buffer, size)
            if woffset >= roffset:
                return buf[roffset:woffset]
            else:
                return buf[:-roffset] + buf[:woffset]
        return [read_buf(addr, *meta) for addr, meta in self.__read_control_block()[0]]

if __name__ == "__main__":
    session = Session(app_ram_start=0x20004718)
    while True:
        for buf in session.read_all():
            sys.stdout.buffer.write(buf)
            sys.stdout.flush()
        time.sleep(0.1)
