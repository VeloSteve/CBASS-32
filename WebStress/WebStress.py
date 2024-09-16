# -*- coding: utf-8 -*-
"""
Web Stress test for CBASS-32

Created on Sat Aug  3 15:06:37 2024

@author: James S. (Steve) Ryan
"""
import urllib.request
import contextlib
from time import sleep
from time import perf_counter
addr1 = "http://192.168.32.26/"
addr2 = "http://192.168.32.28/"
delay = 250  # ms between items
dest = ["", "files", "Tchart.html", "SyncTime", "About", "RampPlan", "LogManagement"]

def stress():
    count = 0
    print("Starting infinite request process.")
    while 1:
        for d in dest:
            tic = perf_counter()
            #contents = urllib.request.urlopen(addr + d)
            with contextlib.closing(urllib.request.urlopen(addr1 + d)) as x:
                pass
            with contextlib.closing(urllib.request.urlopen(addr2 + d)) as x:
                pass            
            count = count + 1
            print(f"Got d = {d} in {(perf_counter() - tic):0.4f} seconds.  Count = {count}")
            sleep(delay / 1000.0)
            
stress()            