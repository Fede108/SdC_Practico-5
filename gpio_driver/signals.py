#!/usr/bin/env python3
import re
import os
import sys
import time
import pexpect

SOCK_PATH = "/tmp/tmp-gpio.sock"
GPIO_RANGE = [0x3f200000, 0x3f200fff]
IC_RANGE   = [0x3f00b200, 0x3f00b3ff]

GPIO_SET_OFFSET   = 0x1c
GPIO_RESET_OFFSET = 0x28
GPIO_READ_OFFSET  = 0x34

# Define two prearmed signals as sequences of (pin_number, value)
SIGNALS = {
    'signal1': [
        (17, 1), (17, 0), (17, 1), (17, 0), (17, 0), (17, 0), (17, 0), (17, 1), (17, 1), (17, 1),
    ],
    'signal2': [
        (4, 1), (4, 0), (4, 1), (4, 0),(4, 1), (4, 0), (4, 1), (4, 0), (4, 0), (4, 0),
    ],
}

# Default delay between samples (seconds)
DEFAULT_DELAY = 1.0

class VGPIOManager(object):
    fd = None

    def __init__(self, spath=SOCK_PATH):
        self.load(spath)

    def load(self, spath=SOCK_PATH):
        if os.path.exists(spath):
            os.unlink(spath)
        self.fd = pexpect.spawn(f"socat - UNIX-LISTEN:{spath}")

    def validate_address(self, address):
        return GPIO_RANGE[0] <= address <= GPIO_RANGE[1]

    def writel(self, address, value):
        self._sendline(f'writel 0x{address:x} 0x{value:x}')
        return self._read()

    def readl(self, address):
        self._sendline(f'readl 0x{address:x}')
        return self._read()

    def read(self, address, size):
        self._sendline(f'read 0x{address:x} 0x{size:x}')
        return self._read()

    def _sendline(self, s):
        return self.fd.sendline(s)

    def _read(self):
        # Cancel echo
        self.fd.readline()
        return self.fd.readline()

    def read_entire_gpio_area(self):
        return self.read(GPIO_RANGE[0], 0x1000)

    def read_ic_area(self):
        return self.read(IC_RANGE[0], 0x200)

    def close(self):
        self.fd.close()

    def help(self):
        s  = "[ ] Virtual GPIO manager\n"
        s += "    Usage:\n"
        s += "    help                                 -- this help message\n"
        s += "    get <gpionum>                        -- read a specific GPIO\n"
        s += "    set <pin> <value>                    -- set a GPIO immediately\n"
        s += "    set <signal_name> [delay_seconds]    -- run a prearmed signal with optional delay (default=1s)\n"
        s += "    toggle <gpionum>                     -- toggle a GPIO value\n"
        s += "    read-area                            -- read entire GPIO area\n"
        s += "    read-ic                              -- read entire interrupt controller area\n"
        s += "    readl <address>                      -- read 32-bit from address\n"
        s += "    writel <address> <value>             -- write 32-bit to address\n"
        s += "    exit                                 -- exit program\n"
        s += "    reload                               -- restart initialization\n"
        return s

    def get_gpio_location(self, num):
        if not (0 <= num <= 54):
            return 0
        # Calculate register base for this GPIO bank
        return GPIO_RANGE[0] + (num // 32) * 4

    def set(self, gpionum, value):
        base = self.get_gpio_location(gpionum)
        offset = GPIO_SET_OFFSET if value else GPIO_RESET_OFFSET
        addr = base + offset
        mask = 1 << (gpionum % 32)
        return self.writel(addr, mask)

    def get(self, gpionum):
        base = self.get_gpio_location(gpionum)
        addr = base + GPIO_READ_OFFSET
        v = int(self.readl(addr).split()[1], 0)
        mask = 1 << (gpionum % 32)
        return bool(v & mask)

    def toggle(self, gpionum):
        current = self.get(gpionum)
        print(f"Toggling GPIO {gpionum}, current value: {current}")
        return self.set(gpionum, not current)

    def run_signal(self, name, delay=DEFAULT_DELAY):
        if name not in SIGNALS:
            print(f"Unknown signal: {name}")
            return
        seq = SIGNALS[name]
        print(f"Starting signal '{name}' with {delay}s delay per sample...")
        for pin, val in seq:
            self.set(pin, val)
            print(f"  -> GPIO {pin} set to {val}")
            time.sleep(delay)
        print(f"Signal '{name}' completed.")

    def parse(self, s):
        parts = s.strip().split()
        if not parts:
            return ''
        cmd = parts[0]

        if cmd == 'help':
            return self.help()
        elif cmd == 'get':
            if len(parts) < 2:
                return "Error: get requires 1 argument"
            return self.get(int(parts[1], 0))
        elif cmd == 'set':
            # run a prearmed signal: set <name> [delay]
            if len(parts) >= 2 and parts[1] in SIGNALS:
                delay = DEFAULT_DELAY
                if len(parts) == 3:
                    try:
                        delay = float(parts[2])
                    except ValueError:
                        return "Error: delay must be a number"
                self.run_signal(parts[1], delay)
                return ''
            # immediate GPIO set: set <pin> <value> 
            if len(parts) >= 3:
                return self.set(int(parts[1], 0), int(parts[2], 0))
            return "Error: set requires 2 args or a valid signal name"
        elif cmd == 'toggle':
            if len(parts) < 2:
                return "Error: toggle requires 1 argument"
            return self.toggle(int(parts[1], 0))
        elif cmd == 'read-area':
            return self.read_entire_gpio_area()
        elif cmd == 'read-ic':
            return self.read_ic_area()
        elif cmd == 'readl':
            if len(parts) < 2:
                return "Error: readl requires 1 argument"
            return self.readl(int(parts[1], 0))
        elif cmd == 'writel':
            if len(parts) < 3:
                return "Error: writel requires 2 arguments"
            return self.writel(int(parts[1], 0), int(parts[2], 0))
        elif cmd == 'reload':
            return self.load()
        elif cmd == 'exit':
            self.close()
            sys.exit(0)
        else:
            return ""

if __name__ == "__main__":
    print('[ ] Virtual GPIO manager')
    print('[ ] Listening for connections')
    vgpio = VGPIOManager()
    while True:
        cmd = input('(gpio)> ')
        output = vgpio.parse(cmd)
        if output is not None:
            print(output)
    
    
