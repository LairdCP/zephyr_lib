## Cellular Profile

### UUID: 43787c60-9e84-4eb1-a669-70b6404da336

Characteristics:

| Name             | UUID                                 | Properties        | Description                                                                                                                                                                       |
| ---------------- | ------------------------------------ | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| IMEI             | 43787c61-9e84-4eb1-a669-70b6404da336 | read              | 15-digit ASCII string representing the IMEI                                                                                                                                       |
| APN              | 43787c62-9e84-4eb1-a669-70b6404da336 | read/write/notify | ASCII string representing the APN (63 characters max). Use an empty string if the APN isn't required.                                                                             |
| APN Username     | 43787c63-9e84-4eb1-a669-70b6404da336 | read/notify       | ASCII string representing the APN username (64 characters max).                                                                                                                   |
| APN Password     | 43787c64-9e84-4eb1-a669-70b6404da336 | read/notify       | ASCII string representing the APN password (64 characters max).                                                                                                                   |
| Network State    | 43787c65-9e84-4eb1-a669-70b6404da336 | read/notify       | One byte. Network state: 0 - Not registered, 1 - Home network, 2 - Searching, 3 - Registration denied, 4 - Out of Coverage, 5 - Roaming, 8 - Emergency, 240 - Unable to configure |
| Firmware Version | 43787c66-9e84-4eb1-a669-70b6404da336 | read              | Firmware version of the LTE modem.                                                                                                                                                |
| Startup State    | 43787c67-9e84-4eb1-a669-70b6404da336 | read/notify       | One byte. Modem startup state: 0 - Ready, 1 - Waiting for access code, 2 - SIM not preset, 3 - Simlock, 4 - Unrecoverable error, 5 - Unknown, 6 - Inactive SIM                    |
| RSSI             | 43787c68-9e84-4eb1-a669-70b6404da336 | read/notify       | Signed 32-bit integer. Reference Signals Receive Power (RSRP) in dBm.                                                                                                             |
| SINR             | 43787c69-9e84-4eb1-a669-70b6404da336 | read/notify       | Signed 32-bit integer. Signal to Interference plus Noise Ratio (SINR) in dBm                                                                                                      |
| Sleep State      | 43787c6a-9e84-4eb1-a669-70b6404da336 | read/notify       | One byte representing sleep state of driver (0 - Uninitialized, 1 - Asleep, 2 - Awake)                                                                                            |
| RAT              | 43787c6b-9e84-4eb1-a669-70b6404da336 | read/write/notify | One byte for Radio Access Technology: 0 - CAT M1, 1 = CAT NB1                                                                                                                     |
| ICCID            | 43787c6c-9e84-4eb1-a669-70b6404da336 | read              | 20-digit ASCII string                                                                                                                                                             |
| Serial Number    | 43787c6d-9e84-4eb1-a669-70b6404da336 | read              | 14 character ASCII string                                                                                                                                                         |
| Bands            | 43787c6e-9e84-4eb1-a669-70b6404da336 | read              | 20 character ASCII string representing LTE band configuration.  See section 5.19 of HL7800 AT command guide for more information.                                                 |
| Active Bands     | 43787c6f-9e84-4eb1-a669-70b6404da336 | read/notify       | 20 character ASCII string representing the Active LTE band configuration.          
