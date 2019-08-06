#!/usr/bin/python
import os
import subprocess
import sys
import getopt
import time
 
# default to the first available bluetooth device
device = "hci0"
name = "DC27Emote"
mfrid = 0x71ff
color = 0x7f
emote = ""
verbose = True
  
if (len(sys.argv) > 1):
    emote = sys.argv[1]

# process a command string - print it if verbose is on,
# and if not simulating, then actually run the command
def process_command(c):
    global verbose
    if verbose:
        print ">>> %s" % c
    os.system(c)

# print status info
print "Advertising on %s with:" % device
print "       name: %s" % name
print "       mfrid: 0x%04x" % mfrid
print "       color: 0x%04x" % color

# Generate the beacon content.
ble_beacon = [
    0x03, 0x19, 0xdc, 0x27,   # Appearance
    0x02, 0x01, 0x06,         # Flags
    len(name) + 1, 0x09,      # Name
]
for ch in name:
    ble_beacon.append(ord(ch))
ble_beacon += [
    len(emote)+5, 0xff,
    (mfrid & 0x00ff) >> 0, (mfrid & 0xff00) >> 8,
    0xb2,
    color
]
for ch in emote:
    ble_beacon.append(ord(ch))

def ble_hex(arr):
    hexstr = ""
    for x in arr:
        hexstr = hexstr + " %02x" % x
    return hexstr

# check to see if we are the superuser - returns 1 if yes, 0 if no
if not 'SUDO_UID' in os.environ.keys():
    print "Error: this script requires superuser privileges.  Please re-run with `sudo.'"
    sys.exit(1)

# first bring up bluetooth
process_command("hciconfig %s up" % device)
# now turn on LE advertising
process_command("hciconfig %s leadv" % device)
# now turn off scanning
process_command("hciconfig %s noscan" % device)
# set up the beacon
# pipe stdout to /dev/null to get rid of the ugly "here's what I did"
# message from hcitool
process_command("hcitool -i hci0 cmd 0x08 0x0008 %02x %s >/dev/null" % (len(ble_beacon), ble_hex(ble_beacon)))
                                                                  
# stop advertisements
time.sleep(3)
process_command("hciconfig %s noleadv" % device)
process_command("hciconfig %s piscan" % device)
process_command("hciconfig %s down" % device)

