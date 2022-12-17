import csv
import matplotlib.pyplot as plt



dir = '../results/work_flow/'
files = ['coarse.csv', 'fine.csv', 'lockfree.csv']


files = list(map(lambda x: dir + x, files))
x = [1, 2, 4, 8, 16, 24, 32, 64, 127]
ys = []

for filename in files:
    # Open the CSV file
    with open(filename, 'r') as f:
        y = []
        # Create a DictReader object
        reader = csv.DictReader(f)
        
        # Iterate over the rows of the CSV file
        for row in reader:
            # Print the values for each column
            y_str = row['Ops/s']
            try:
                y_val = int(y_str)
            except ValueError:
                y_val = 0
            y.append(y_val)
    ys.append(y)

print(ys)
plt.autoscale(True)
    
plt.plot(x, ys[0], label="coarse-grained")
plt.plot(x, ys[1], label ="fine-grained")
plt.plot(x, ys[2], label = "lock-free")
plt.legend()
plt.ylim((0, 1e8))
plt.xlabel("Thread count")
plt.ylabel("Ops/second")
plt.savefig(dir+"result.png", dpi=300)
plt.clf()

        