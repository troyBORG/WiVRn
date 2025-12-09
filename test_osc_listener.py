#!/usr/bin/env python3
from pythonosc import osc_server
from pythonosc.dispatcher import Dispatcher

def print_handler(address, *args):
    print(f"{address}: {args}")

dispatcher = Dispatcher()
dispatcher.map("/tracking/trackers/*", print_handler)

server = osc_server.ThreadingOSCUDPServer(("127.0.0.1", 9000), dispatcher)
print("Listening for OSC messages on 127.0.0.1:9000")
print("Press Ctrl+C to stop")
server.serve_forever()
