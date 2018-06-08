# NavCoin v4.2.1 Release Notes

### Hot Fix:

- The wallet will try to synchronize clock using a NTP Server every 30 seconds on launch until it succeds instead of just checking once and exiting if it fail
- Change the buffer size of the socket reading from the NTP Server to 32 bit, fixing some compatibiity issues with 32 bit systems.
