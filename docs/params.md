
# Parameters
* Averaging window (in ms)
* Listening interface
* Command port

# Commands
All values are bytes. `IP3` represents the first number in IP address (most significant).

`0x0 | SIP3 | SIP2 | SIP1 | SIP0 | SPORT | DIP3 | DIP2 | DIP1 | DIP0 | DPORT`
    * Starts grooming a stream from SIP:SPORT to DIP:DPORT

`0x1 | ....`
    * Stops grooming a stream from SIP:SPORT. Same format as command 0, without destination
      information.

`0x2 | MS`
    * Sets the smoothing window in tens of milliseconds.
