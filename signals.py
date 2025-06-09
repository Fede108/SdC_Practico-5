#!/usr/bin/env python3
import os
import sys
import time
import threading
import pexpect

SOCK_PATH = "/tmp/tmp-gpio.sock"
GPIO_RANGE = [0x3f200000, 0x3f200fff]
IC_RANGE   = [0x3f00b200, 0x3f00b3ff]

GPIO_SET_OFFSET   = 0x1c
GPIO_RESET_OFFSET = 0x28
GPIO_READ_OFFSET  = 0x34

# Define two prearmed signals as sequences of (pin_number, value)
SIGNALS = {
    'signal1': [ (17,1),(17,1),(17,1),(17,0),(17,0),(17,1),(17,1),(17,1),(17,0),(17,0) ],
    'signal2': [ (4,1),(4,0),(4,1),(4,0),(4,1),(4,0),(4,1),(4,0),(4,1),(4,0),(4,1) ],
}

# Default delay (seconds)
DEFAULT_DELAY = 1.0

class VGPIOManager:
    def __init__(self, spath=SOCK_PATH):
        self.spath = spath
        self.fd = None
        self.loop_thread = None
        self.loop_flag = threading.Event()
        self.load()

    def load(self):
        if os.path.exists(self.spath): os.unlink(self.spath)
        self.fd = pexpect.spawn(f"socat - UNIX-LISTEN:{self.spath}")

    def close(self):
        self.stop_loop()
        self.fd.close()

    def writel(self, address, value):
        self.fd.sendline(f'writel 0x{address:x} 0x{value:x}')
        self.fd.readline(); return self.fd.readline()

    def readl(self, address):
        self.fd.sendline(f'readl 0x{address:x}')
        self.fd.readline(); return self.fd.readline()

    def get_gpio_location(self, num):
        if not 0 <= num <= 54: return 0
        return GPIO_RANGE[0] + (num//32)*4

    def set_gpio(self, gpionum, value):
        base = self.get_gpio_location(gpionum)
        offset = GPIO_SET_OFFSET if value else GPIO_RESET_OFFSET
        addr = base + offset
        mask = 1 << (gpionum % 32)
        return self.writel(addr, mask)

    def run_signal_once(self, name, delay):
        seq = SIGNALS[name]
        for pin,val in seq:
            if not self.loop_flag.is_set(): break
            self.set_gpio(pin,val)
            time.sleep(delay)

    def loop_signal(self, name, delay):
        print(f"Looping '{name}' every {delay}s. Use 'stop' or 'set <otra>' to change.")
        self.loop_flag.set()
        while self.loop_flag.is_set():
            self.run_signal_once(name, delay)
        print(f"Stopped looping '{name}'.")

    def stop_loop(self):
        self.loop_flag.clear()
        if self.loop_thread and self.loop_thread.is_alive():
            self.loop_thread.join()
        self.loop_thread = None

    def help(self):
        return ("Virtual GPIO manager\n"
                " Usage:\n"
                " help                                show this message\n"
                " get <gpionum>                       read a GPIO\n"
                " set <pin> <value>                   immediate set\n"
                " set <signal> [delay]                start looping signal\n"
                " stop                                 stop looping signal\n"
                " toggle <gpionum>                    toggle GPIO\n"
                " read-area                           read entire GPIO area\n"
                " read-ic                             read interrupt controller area\n"
                " exit                                exit program\n")

    def parse(self, s):
        parts = s.strip().split()
        if not parts: return ''
        cmd = parts[0]
        if cmd == 'help': return self.help()
        if cmd == 'get': return self.readl(self.get_gpio_location(int(parts[1],0))+GPIO_READ_OFFSET)
        if cmd == 'toggle': return self.set_gpio(int(parts[1],0), not bool(int(self.get(int(parts[1],0)))) )
        if cmd == 'set':
            if len(parts)>=2 and parts[1] in SIGNALS:
                delay = DEFAULT_DELAY
                if len(parts)==3:
                    try: delay=float(parts[2])
                    except: return "Delay must be number"
                self.stop_loop()
                self.loop_thread = threading.Thread(target=self.loop_signal,args=(parts[1],delay),daemon=True)
                self.loop_thread.start()
                return ''
            if len(parts)>=3:
                return self.set_gpio(int(parts[1],0),int(parts[2],0))
            return "Error: set needs <pin> <val> or <signal>"
        if cmd == 'stop':
            self.stop_loop(); return ''
        if cmd == 'exit':
            self.close(); sys.exit(0)
        return ''

if __name__=="__main__":
    vgpio = VGPIOManager()
    print(vgpio.help())
    while True:
        cmd = input('(gpio)> ')
        try:
            out = vgpio.parse(cmd)
        except Exception as e:
            print(f"Error: {e}")
            continue
        if out:
            print(out)
