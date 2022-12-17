import subprocess
import re
import csv



cores = [1, 2, 4, 8, 16, 24, 32, 64, 128]
local_cores = [1, 2, 4, 6]



def find_ops_per_second(output):
    output_str = output.decode()
    i = output_str.find("Ops/s:")
    output_str = output_str[i:]
    end = output_str.find("\n")
    output_str = output_str[8:end]
    return output_str


def run_benchmark(filename):
    with open(filename+".csv", "w", newline="") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["Thread count", "Ops/s"])
        writer.writeheader()

        for core_cnt in local_cores:
            output = subprocess.check_output(["./" + filename, str(core_cnt)])
            speed = find_ops_per_second(output)
            writer.writerow({"Thread count": str(core_cnt), "Ops/s": speed})


if __name__ == "__main__":
    coarse_grained_cmd = "g++ -O2 -std=c++17 -pthread -Wall -o coarse benchmark-coarse-grained.cpp".split(' ')
    fine_grained_cmd = "g++ -O2 -std=c++17 -pthread -Wall -o fine benchmark-heap.cpp".split(' ')
    lock_free_cmd = "g++ -O2 -std=c++17 -pthread -Wall -o lockfree benchmark-lock-free.cpp".split(' ')

    subprocess.call(coarse_grained_cmd)
    subprocess.call(fine_grained_cmd)
    subprocess.call(lock_free_cmd)


    run_benchmark("coarse")

   

    

    
    








    subprocess.call("rm coarse".split( ))
    subprocess.call("rm fine".split( ))
    subprocess.call("rm lockfree".split( ))


