# Laird Connectivity Zephyr Module - Key Value Pair (KVP)

This module parses and generates key-value pair files.

The format of the files is name=value\n. The '=' and '\n' characters can't be used in names or values. Carriage returns are accepted but stripped by the parser.

```
# The comment character '#' must be the first character on a line.
name=sensor
rate=10
# Values for numbers can be in decimal or hex.
dog=0x54
pin=1234567
# Values for byte arrays must be in hexadecimal (least significant byte first).
secret=0102030405060708090a0b0c
# Empty strings use "" because each key must have a value with a length greater than 1
location=""

```
