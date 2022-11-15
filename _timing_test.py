#/usr/local/bin/python3
import subprocess
import random
import time
from alive_progress import alive_bar
import numpy as np

NUM_TESTS = 250

tests = random.sample(range(49152), NUM_TESTS)
times = []

try:
    with alive_bar(NUM_TESTS) as progress_bar:
        for test in tests:
            subprocess.call(["cp", f"./royle-puzzles/puzzle{test:05d}.txt", "puzzle_in.txt"])
            start_time = time.time()
            subprocess.call(["./Sudoku"])
            times.append( time.time() - start_time )
            progress_bar()
except KeyboardInterrupt:
    pass
except Exception as e:
    print(e)
finally:
    if len(times) != 0:
        times.sort()
        print(f"Min. Time:   {min(times):.6f}")
        print(f"Avg. Time:   {sum(times)/len(times):.6f}")
        print(f"Median:      {(times[len(times)//2]+times[(len(times)-1)//2])/2:.6f}")
        for pctl in (75, 90, 95, 99):
            print(f"{pctl:02d}th Pctl.:  {np.percentile(times, pctl):.6f}")
        print(f"Max. Time:   {max(times):.6f}")
        print(f"Total Time:  {sum(times):.6f}")
        print(f"Sample Size: {len(times):d}")
    else:
        print("\nTerminated before solving any puzzles")
    print('\n')
