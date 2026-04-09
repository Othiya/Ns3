

import matplotlib.pyplot as plt
from pathlib import Path


vegas_files = ["plots2/TcpVegas-basic-cwnd.dat", "plots2/TcpVegas-basic-queue.dat", "plots2/TcpVegas-basic-throughput.dat", "plots2/TcpVegas-basic-length.dat"]
quick_vegas_files = ["plots2/TcpQuickVegas-basic-cwnd.dat", "plots2/TcpQuickVegas-basic-queue.dat", "plots2/TcpQuickVegas-basic-throughput.dat", "plots2/TcpQuickVegas-basic-length.dat"]

labels = ["CWND", "Queue Size", "Throughput", "Queue Length"]
y_labels = ["CWND (packets)", "Queue Size (packets)", "Throughput (Mb/s)", "Queue Length"]


for idx, (vegas_file, quick_vegas_file, label, y_label) in enumerate(zip(vegas_files, quick_vegas_files, labels, y_labels)):
    vegas_times = []
    vegas_values = []
    quick_vegas_times = []
    quick_vegas_values = []
    
    
    with open(vegas_file, "r") as f:
        for line in f:
            line = line.strip()
            if line == "" or line.startswith("#"):
                continue
            t, val = map(float, line.split())
            vegas_times.append(t)
            vegas_values.append(val)
    
    
    with open(quick_vegas_file, "r") as f:
        for line in f:
            line = line.strip()
            if line == "" or line.startswith("#"):
                continue
            t, val = map(float, line.split())
            quick_vegas_times.append(t)
            quick_vegas_values.append(val)


    fig, axes = plt.subplots(1, 2, figsize=(14, 6))  
    
   
    axes[0].plot(vegas_times, vegas_values, label="Vegas", color='blue')
    axes[0].set_xlabel("Time (s)")
    axes[0].set_ylabel(y_label)
    axes[0].set_title(f"NS-3 Simulation Plot: {label} (Vegas)")
    axes[0].legend()
    axes[0].grid(True)


    axes[1].plot(quick_vegas_times, quick_vegas_values, label="Quick Vegas", color='red')
    axes[1].set_xlabel("Time (s)")
    axes[1].set_ylabel(y_label)
    axes[1].set_title(f"NS-3 Simulation Plot: {label} (Quick Vegas)")
    axes[1].legend()
    axes[1].grid(True)

 
    plt.tight_layout()
    

    fig.savefig(f"plots2/{label}_comparison.svg", dpi=200)
    plt.close(fig)

print("SVG files saved successfully!")