#!/usr/bin/python

import struct
import socket
import traceback

class TCPInfo:
    def __init__(self, data):
        n = struct.unpack('='
                          + 7 * 'B'
                          + 'x'
                          + 24 * 'I', data)

        self.keys = [
            'state',
            'ca_state',
            'retransmits',
            'probes',
            'backoff',
            'options',
            'wscale',

            'rto',
            'ato',
            'snd_mss',
            'rcv_mss',

            'unacked',
            'sacked',
            'lost',
            'retrans',
            'fackets',

            'last_data_sent',
            'last_ack_sent',
            'last_data_recv',
            'last_ack_recv',

            'pmtu',
            'rcv_ssthresh',
            'rtt',
            'rttvar',
            'snd_ssthresh',
            'snd_cwnd',
            'advmss',
            'reordering',
            'rcv_rtt',
            'rcv_space',
            'total_retrans',
            ]

        self.data = {}
        for i,k in zip(range(len(self.keys)), self.keys):
            self.data[k] = n[i]

def mainloop(fd):
    while True:
        try:
            data,addr = fd.recvfrom(8192)
            tcp = TCPInfo(data[:104])
            for k in tcp.keys:
                print "%-15s %d" % (k, tcp.data[k])
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            traceback.print_exc()
        
def main():
    host = ''
    port = 12346
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, port))
    mainloop(s)

if __name__ == '__main__':
    main()
