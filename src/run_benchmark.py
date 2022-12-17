import subprocess
import csv
# import matplotlib.pyplot as plt

cores = [1, 2, 4, 8, 16, 24, 32, 64, 127]
# cores = [1, 2, 3, 4]

def find_ops_per_second(output):
    output_str = output.decode()
    i = output_str.find("Ops/s:")
    output_str = output_str[i:]
    end = output_str.find("\n")
    output_str = output_str[8:end]
    return output_str


def run_benchmark(filename):
    y = []
    with open(filename+".csv", "w", newline="") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=["Thread count", "Ops/s"])
        writer.writeheader()

        for core_cnt in cores:
            output = subprocess.check_output(["./" + filename, str(core_cnt)])
            speed = find_ops_per_second(output)
            writer.writerow({"Thread count": str(core_cnt), "Ops/s": speed})
            y.append(int(speed))

        return y
      


if __name__ == "__main__":
    coarse_grained_cmd = "g++ -O2 -std=c++17 -pthread -Wall -o coarse reaction-diffusion-coarse-grained.cpp".split(' ')
    fine_grained_cmd = "g++ -O2 -std=c++17 -pthread -Wall -o fine reaction-diffusion-heap.cpp".split(' ')
    lock_free_cmd = "g++ -O2 -std=c++17 -pthread -Wall -o lockfree reaction-diffusion-lock-free.cpp".split(' ')

    subprocess.call(coarse_grained_cmd)
    subprocess.call(fine_grained_cmd)
    subprocess.call(lock_free_cmd)

    y1 = run_benchmark("coarse")
    # y2 = []
    # y3 = []
    y2 = run_benchmark("fine")
    y3 = run_benchmark("lockfree")


    # plt.autoscale(True)
    
    # plt.plot(cores, y1, label="coarse-grained")
    # plt.plot(cores, y2, label ="fine-grained")
    # plt.plot(cores, y3, label = "lock-free")
    # plt.legend()
    # plt.xlabel("Thread count")
    # plt.ylabel("Ops/second")
    # plt.savefig("test.png", dpi=300)
    # plt.clf()

    subprocess.call("rm coarse".split( ))
    subprocess.call("rm fine".split( ))
    subprocess.call("rm lockfree".split( ))


