# Project 2 - Reliable Data Transfer 

- Name: Andy Kempf
- Email: AndyKempf@u.boisestate.edu
- Class: 425-001

## Design

The project is seperated into 3-layers, as per specified in the PDF files. The layers are as follows:
- Packet Layer : Defines the type and size of packets, computed the checksum for each packet, encodes packets into wire bytes, and decodes/validates incoming datagrams. This layer generate the information that a packet is.
- Protocol Layer : Implements Go-Back-N behavior. While individual packets are sent, the Protocol Layer will track the status of those packets, in what order they are acknowledged, after how logn they should be resent, etc... This layer orchestrates the file transfer and ensures that the data is all received. If the sender's timer expires before an acknowledgement advances the window, the sender retransmits all outstanding packets.
- I/O Layer : This is the component that actually sends the packets. While the packet layer handles generating packet data, the protocol layer orchestrates which packets should be sent, and ensures they are all properly received, the I/O layer includes the machinery used to actually send the packets over the "network". It is responsible for allocating the actual resources (sockets, files, etc...) used to transmit the packet data. It also where the top-level interaction for the software comes from, it reads command-line arguments and opens the UDP socket needed, and registers that socket with the relay. 

The three layer split exists to consolidate responsibilities, for instance, the packet data layer doesn't need to know about how sockets are handled, and the Protocol Layer doesnt need to know what exactly is contained in each particular packet.

The three layer split also allows us to test each component individually, without needing to simulate the entire pipeline.

Files in each layer:
- Packet Layer: `packets.h`/`packets.c`
- Protocol Layer: `gbn_protocol.h`/`gbn_protocol.c`
- I/O Layer: `transfer_io.h`/`transfer_io.c`

## Results

| Window | Loss | Corrupt | Dup | Mean time (s) | Throughput (KiB/s) |
|---|---|---|---|---|---|
| 1 | 0 | 0 | 0 | 103.567 | 9.887 |
| 16 | 0 | 0 | 0 | 6.631 | 154.415 |
| 1 | 0.05 | 0 | 0 | 129.399 | 7.914 |
| 16 | 0.05 | 0 | 0 | 23.623 | 43.347 |

## Results Analysis

With a window of 1 and no loss, the mean transfer time was 103.567 seconds. Dividing this by the 1025 packets gives an estimated round-trip time of `101.04 ms`. The relay contributes 100ms, which leaves an extra 1.04 ms spend on operating-system effects, relay processing, and socket/file delays. Increasing the window to 16 reduced the mean time to 6.631 seconds, a `15.62` times speedup. This is proportional to the window size because the larger window allows the sender to keep multiple packets in flight while waiting for `ACK`s to come back. 

With 5% loss, the window-1 time increased from 103.567 to 129.399 seconds, approximated a 25% increase. The window-16 time increased from 6.631 to 23.623 seconds, an almost 256% increase. Lost packets or acknowledgements can cause the sender's timer to expire. Go-Back-N then tells the sender to retransmit every outstanding packet within the current window. Window 1 will retransmit only a single packets, while window 16 could retransmit multiple. The receiver also discard packets that arrive beyond a gap, requiring them to be sent again. This makes loss proportionally more expensive when the window size is larger, though a window size of 16 remains much faster. 

## Testing

The Unity tests cover packet encoding and decoding, checksum examples, corrupted packets, malformed headers, and invalid lengths. 

Receiver tests check ordering, duplicated, gaps, FIN, and timeouts.

Sender tests check window limits, cumulative acknowledgements, retransmission, and failure after ten timeouts with no progress.

Complete transers are tested usinga f ake clock and in-memory chanel with 20% loss (seeded for consistent behavior). Delivered bytes are compared against the original input, Tests include empty files and files with sizes that are exact multiples of 1024 bytes.

All 17 of the tests pass and line coverage is 100%.

## known Bugs or Issues

No known bugs were found in unit testing, sanitizer checking, or relay transfers.

## Experience

I began the project by using the printscreen features to turn the assignments canvas page into a PDF. I include the PDF in the project directory and have Codex review it in plan mode. I go over with codex how I want files seggregated, how I want the code formatted and documented, and introduce it to personal style choices like the use of regions and seperating brackets and long paran arguments onto their own newlines.

For the table results, I asked codex to create a python script that would automatically run all of the benchmark scenarios then export the table to a markdown file.