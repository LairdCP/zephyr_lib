## Power Profile

### UUID: dc1c0000-f3d7-559e-f24e-78fb67b2b7eb

Characteristics:

| Name                 | UUID                                 | Properties | Description                                                                                                                                                      |
| -------------------- | ------------------------------------ | ---------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Power supply voltage | dc1c0001-f3d7-559e-f24e-78fb67b2b7eb | notify     | Two bytes. Byte 0 is the integer part of the voltage and byte 1 is the decimal part of the voltage                                                               |
| Reboot               | dc1c0002-f3d7-559e-f24e-78fb67b2b7eb | write      | One bytes. Writing to this will reboot the module, writing a value of 0x01 will stay in the UART bootloader, any other value will reboot to the user application |
