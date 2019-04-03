#!/usr/bin/env bcc-py3

import os
import subprocess

pids = [pid for pid in os.listdir('/proc') if pid.isdigit()]

for pid in pids:
    try:
        # The following command will probably give us all probes including usdt. Sheesh. Just filter them out.
        probes = subprocess.run(['/tmp/bpftrace', '-l', '-p', pid], stdout=subprocess.PIPE)
        plist = probes.stdout.decode('utf-8').split()

        print("pid ", pid)
        for p in plist:
            print(p)
            if p.startswith('usdt:'):
                print(p)
    except subprocess.CalledProcessError as e:
        print(e.output)
        continue

