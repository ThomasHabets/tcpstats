#!/usr/bin/python

#from twisted.application.internet import MulticastServer

from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor
import struct
import re
import sqlite3

DATA = [
    'version',
    'serial',
    'peerv4',
    'peerv6',
    'localv4',
    'localv6',
    'peerport',
    'localport',

    'state',
    'ca_state',
    'retransmits',
    'probes',
    'backoff',
    'options',
    'snd_wscale',
    'rcv_wscale',

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

def process_tlv(name, dataformat):
    formatter = struct.Struct(dataformat)
    def func1(func):
        def func2(obj, tlv):
            if len(tlv[2]) != formatter.size:
                raise ErrBase(name,
                              "len %d, should be %d" % (len(tlv[2]),
                                                        formatter.size))
            return func(obj, tlv, *formatter.unpack(tlv[2]))
        return func2
    return func1

class MulticastServerUDP(DatagramProtocol):
    def ErrTLV(Exception):
        def __init__(self, tlv, msg):
            pass

    def __init__(self, db):
        self.db = sqlite3.connect("tcpstats.sqlite")
        self.cursor = self.db.cursor()

    def startProtocol(self):
        self.transport.joinGroup('224.1.2.3')
        #self.transport.joinGroup('[ff05::4711]')

    def datagramReceived(self, datagram, address):
        #self.transport.write("data", address)
        #print "Data: ",repr(datagram)
        data = datagram
        tlvs = []
        while len(data):
            (t,l),rest = struct.unpack("!HH", data[:4]),data[4:]
            v,data = rest[:l],rest[l:]
            tlvs.append( (t,l,v) )
        self.process_tlvs(tlvs)

    @process_tlv('version', 'B')
    def process_tlv_version(self, tlv, version):
        if version != 1:
            raise self.ErrTLV("version", "version != 1")
        print "Version:",version
        return {}

    @process_tlv('serial', '!Q')
    def process_tlv_serial(self, tlv, serial):
        print "Serial: %d" % (serial)
        return {'serial': serial}

    @process_tlv('rtt', '!I')
    def process_tlv_rtt(self, tlv, rtt):
        print "RTT:", rtt
        return {'rtt': rtt}

    @process_tlv('rcv_rtt', '!I')
    def process_tlv_rcv_rtt(self, tlv, rcv_rtt):
        print "RCV RTT:", rcv_rtt
        return {'rcv_rtt': rcv_rtt}

    @process_tlv('pmtu', '!I')
    def process_tlv_pmtu(self, tlv, pmtu):
        print "PMTU:", pmtu
        return {'pmtu': pmtu}

    @process_tlv('rto', '!I')
    def process_tlv_rto(self, tlv, rto):
        print "RTO:", rto
        return {'rto': rto}

    def process_tlv_peerv4_backend(self, tlv, v4addr):
        print "Peer v4:",v4addr
        return {'peer': v4addr}

    @process_tlv('peerv4', 'BBBB')
    def process_tlv_peerv4(self, tlv, a,b,c,d):
        return self.process_tlv_peerv4_backend(tlv, "%d.%d.%d.%d" % (a,b,c,d))

    @process_tlv('peerv6', '16s')
    def process_tlv_peerv6(self, tlv, peer):
        if peer[:12] == "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF":
            arr = tuple([int(ord(x)) for x in peer[12:16]])
            return self.process_tlv_peerv4_backend(tlv, "%d.%d.%d.%d" % arr)

        def bytes2addr(b):
            t = ''.join(["%.2x" % (ord(x)) for x in b])
            return re.sub("(....)", r"\1:", t)[:-1]
        s = bytes2str(peer)
        print "Peer v6:", str
        return {'peer': str}

    def process_tlv(self, tlv):
        return {'version': self.process_tlv_version,
                'serial':  self.process_tlv_serial,
                'peerv4':  self.process_tlv_peerv4,
                'peerv6':  self.process_tlv_peerv6,
                'rtt':     self.process_tlv_rtt,
                'rcv_rtt':     self.process_tlv_rcv_rtt,
                'pmtu':     self.process_tlv_pmtu,
                'rto':     self.process_tlv_rto,
                }[DATA[tlv[0]]](tlv)

    def process_tlvs(self, tlvs):
        data = {}
        for tlv in tlvs:
            data.update(self.process_tlv(tlv))
        self.cursor.execute("""
INSERT INTO
  tcpstats
(
  tcpstatsid,
  pserial,
  peer,
  rtt,
  rto,
  rcv_rtt,
  pmtu
) VALUES (
  NULL,
  %(serial)d,
  '%(peer)s',
  %(rtt)d,
  %(rto)d,
  %(rcv_rtt)d,
  %(pmtu)d
)
""" % data)
        self.db.commit()
        print "-----------"
# Note that the join function is picky about having a unique object
# on which to call join.  To avoid using startProtocol, the following is
# sufficient:
#reactor.listenMulticast(8005, MulticastServerUDP()).join('224.0.0.1')

def main():
    # Listen for multicast on 224.1.2.3:12346
    reactor.listenMulticast(12346, MulticastServerUDP("tcpstats.sqlite"))
    reactor.run()

if __name__ == '__main__':
    main()
