DROP TABLE tcpstats;
CREATE TABLE tcpstats(
       tcpstatsid integer primary key,
       pserial integer not null,
       peer text not null,
       pmtu integer not null,
       rtt integer not null,
       rcv_rtt integer not null,
       rto integer not null);
