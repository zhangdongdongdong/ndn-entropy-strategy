#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
from subprocess import call
import sys
import subprocess
sys.path.append("/home/duoduo/workerpool-0.9.4")
import workerpool
import multiprocessing

class SimulationJob (workerpool.Job):
    "Job to simulate things"
    def __init__ (self, cmdline):
        self.cmdline = cmdline
    def run (self):
        print self.cmdline
        subprocess.call (self.cmdline,shell=True)
        
        
pool = workerpool.WorkerPool(size = multiprocessing.cpu_count())

runs = range(1,51)
for run in runs:

#NS_LOG=ndn.Face:ndn.Consumer 

#------------- 7 node start-----------------

 
 cmdline = ["./waf --run='tree-zoom-ndn-entropyRouting --run=%d'" % run]
 job = SimulationJob (cmdline)
 pool.put (job)
 
 pool.join ()
pool.shutdown ()  
