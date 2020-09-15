## FOTA Profile

### UUID: 3e12f000-0a2a-32tb-2b85-8349747c5745

Characteristics:

| Name             | UUID                                 | Properties        | Description
| ---------------- | ------------------------------------ | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Control Point    | 3e12f001-0a2a-324b-2685-8349747c5745 | read/write/notify | One byte for controlling the FOTA service. 0 - NOP, 1 - List Files, 2 - Modem Start, 3 - Delete Files, 4 - Compute SHA256                                                                                                                                 | 
| Status           | 3e12f002-0a2a-324b-2685-8349747c5745 | read/notify       | Integer. Negative System error code. 0 - Success, 1 - Busy, 2 - Unspecific error code.  Any value other than 1 can be considered Idle.  Busy will always be notified.  Any subsequent commands issued before a success or error response will be ignored. |
| Count            | 3e12f003-0a2a-324b-2685-8349747c5745 | read/notify       | The number of bytes that have been transferred in the current FOTA update.  This may be larger than the size if data is padded during transfer.                                                                                                           |
| Size             | 3e12f004-0a2a-324b-2685-8349747c5745 | read/notify       | The size of the file in bytes.                                                                                                                                                                                                                            |
| File Name        | 3e12f005-0a2a-324b-2685-8349747c5745 | read/write/notify | The file name of the current operation.  File names are pattern matched.  An empty string will match all files.  The filesystem is not traversed.                                                                                                         |
| Hash             | 3e12f006-0a2a-324b-2685-8349747c5745 | read/notify       | The 32-byte SHA256 hash of the current file.                                                                                                                                                                                                              |
