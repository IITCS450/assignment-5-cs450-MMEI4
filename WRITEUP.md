2 consumers run sucessfully run concurrently in consuming 100 itmems through an eight slot buffer and one mutex protector.
<img width="1210" height="946" alt="Screenshot 2026-03-30 at 3 00 19 PM" src="https://github.com/user-attachments/assets/6a6d4776-5a64-4c9e-83c0-33e578c64ec3" />

The threads do not run into deadlocks, and the consumer's last byte value match the producer encoding of (1*1000 +i), likewise with the other consumer (2*1000 +i).
Each consumers also reach the same progress which confirms the RR scheduler.
