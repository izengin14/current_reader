#This script is in combo with the "output.txt" and "vddq_current_plot.png"
#It will track VDDQ current for around 4 seconds and save the current to output.txt and plot it over time in .png
#To run this program, input command line "python3 toOutputTxt.py <num of iteration"; if no input of num of ietration, it will default 10000;
#For reference 10000 iteration is long enough to collect data for 8 binary bits for transfer rate of 500

import time
import pandas as pd
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from pandas.plotting import register_matplotlib_converters
register_matplotlib_converters()
from matplotlib.ticker import MultipleLocator, FormatStrFormatter, AutoMinorLocator
import sys
from datetime import datetime

# Paths to monitor
curr_paths = {
    "VDDQ Current": "/sys/bus/i2c/drivers/ina3221/1-0041/hwmon/hwmon4/curr2_input",  #DRAM current
    #"/sys/bus/i2c/drivers/ina3221/1-0040/hwmon/hwmon3/in2_input",  #CPU and CV voltage
    #"/sys/bus/i2c/drivers/ina3221/1-0040/hwmon/hwmon3/in1_input", #GPU voltage
    #,
    # Add more sensors here if needed
    "GPU and SOC Current": "/sys/bus/i2c/drivers/ina3221/1-0040/hwmon/hwmon3/curr1_input", #GPU current
    "CPU and CV Current": "/sys/bus/i2c/drivers/ina3221/1-0040/hwmon/hwmon3/curr2_input",  #CPU current
    #


}

# Data collection
records = []

#clear the output.txt before apprend
open("output.txt", "w").close()

# Get number of iterations from command-line argument, default to 10000 if not provided
try:
    num_seconds = int(sys.argv[1])
except (IndexError, ValueError):
    num_seconds = 10  # default value if no valid argument is provided

print(f"Running with {num_seconds} seconds...")


start_time = time.time()
elapsed_time = time.time() - start_time


start_time_sys=datetime.now().strftime('%H:%M:%S.%f')


while elapsed_time<=num_seconds:
#for i in range(num_iterations):
    #To print timestamp
    #row = {"timestamp": datetime.now().strftime('%H:%M:%S.%f')}  

    #To print elapsed time
    elapsed_time = time.time() - start_time
    row = {"elapsed_time": elapsed_time}

    for label, path in curr_paths.items():
        try:
            with open(path, 'r') as f:
                value = int(f.read().strip())
                row[label] = value
        except Exception as e:
            row[label] = None  # or some error code

    # Open output file in append mode
    with open("output.txt", "a") as output_file:
         # Write the row to output.txt
         output_file.write(f"{row['elapsed_time']},{row.get('VDDQ Current', 'None')}\n")
    
    records.append(row)

    # Display and wait
    print(row)
    #time.sleep(0.00000001)  # Change interval as needed




print("Stopping data collection...")




df = pd.DataFrame(records)

# Combined Plot for All Currents
plt.figure(figsize=(40, 8))

plt.plot(df['elapsed_time'], df['VDDQ Current'], label='VDDQ Current', color='blue')
plt.plot(df['elapsed_time'], df['GPU and SOC Current'], label='GPU and SOC Current', color='red')
plt.plot(df['elapsed_time'], df['CPU and CV Current'], label='CPU and CV Current', color='green')

# Set major and minor ticks
plt.gca().xaxis.set_major_locator(MultipleLocator(0.5))
plt.gca().xaxis.set_major_formatter(FormatStrFormatter('%.2f'))
plt.gca().xaxis.set_minor_locator(MultipleLocator(0.1))

# Enable grid
plt.grid(True, which='major', linestyle='-', linewidth=0.75)
plt.grid(True, which='minor', linestyle='--', linewidth=0.3, alpha=0.5)

# Labels and title
plt.xlabel('Time (s)', fontsize=20)
plt.ylabel('Current (mA)', fontsize=20)
plt.title('Current Over Time')
plt.legend(loc='upper right')
plt.xticks(rotation=45, fontsize=20)
plt.legend(fontsize=18)
plt.yticks(fontsize=20)
plt.tight_layout()

# Save and show
plt.savefig("combined_current_plot.png")
print("Combined plot saved as combined_current_plot.png")
plt.show()


#The code to plot three graphs seperately
# Create the plot for DRAM
plt.figure(figsize=(40, 6))

plt.plot(df['elapsed_time'], df['VDDQ Current'], label='VDDQ Current', color='blue')

# Add vertical lines for "1" bits from ones.txt 
with open("ones.txt", "r") as f:
    raw_lines = [line.strip() for line in f if line.strip()]
  
    ones_us = raw_lines[1:]  # Skip the first element
   
    adjusted_times=[datetime.strptime(ts,"%H:%M:%S.%f")-datetime.strptime(start_time_sys,"%H:%M:%S.%f") for ts in ones_us]


    adjusted_ones_seconds=[td.total_seconds() for td in adjusted_times]

    for i, ts in enumerate(adjusted_ones_seconds):
        plt.axvline(x=ts, color='orange', linestyle='--', linewidth=4,
                    label='1-bit' if i == 0 else "")  # Label only the first for legend
        
# Add vertical lines for "0" bits from zeros.txt
with open("zeros.txt", "r") as f:
    zeros_us = [line.strip() for line in f if line.strip()]

    adjusted_times=[datetime.strptime(ts,"%H:%M:%S.%f")-datetime.strptime(start_time_sys,"%H:%M:%S.%f") for ts in zeros_us]

    adjusted_zeros_seconds=[td.total_seconds() for td in adjusted_times]

    for i, ts in enumerate(adjusted_zeros_seconds):
        plt.axvline(x=ts, color='grey', linestyle='--', linewidth=4,
                    label='0-bit' if i == 0 else "")  # Label only the first for legend
        
# Add vertical lines for "1" and "0" bits from ends.txt
with open("ends.txt", "r") as f:
    raw_lines = [line.strip() for line in f if line.strip()]
  
    ends_us = raw_lines[-1:]  # keep the last element

    adjusted_times=[datetime.strptime(ts,"%H:%M:%S.%f")-datetime.strptime(start_time_sys,"%H:%M:%S.%f") for ts in ends_us]

    adjusted_ends_seconds=[td.total_seconds() for td in adjusted_times]

    for i, ts in enumerate(adjusted_ends_seconds):
        plt.axvline(x=ts, color='black', linestyle='--', linewidth=4,
                    label='Last bit finish' if i == 0 else "")  # Label only the first for legend

# Set major ticks every 1 millisecond (0.001 seconds)
plt.gca().xaxis.set_major_locator(MultipleLocator(0.5))
plt.gca().xaxis.set_major_formatter(FormatStrFormatter('%.2f'))  # Format to 3 decimal places

# Set minor ticks every 0.01 seconds (for grid lines)
plt.gca().xaxis.set_minor_locator(MultipleLocator(0.1))

# Enable grid for both major and minor ticks
plt.grid(True, which='major', linestyle='-', linewidth=0.75)
plt.grid(True, which='minor', linestyle='--', linewidth=0.3, alpha=0.5)

plt.xlabel('Time(s)', fontsize=20)
plt.ylabel('VDDQ Current', fontsize=20)
plt.title('VDDQ Current Over Time')
plt.xticks(rotation=45, fontsize=20)
plt.yticks(fontsize=20)
plt.legend(fontsize=18)
plt.grid(True)
plt.tight_layout()

# Save plot as PNG
plt.savefig("vddq_current_plot.png")
print("Plot saved as vddq_current_plot.png")

plt.show()  # Optionally display the plot





# Plot for GPU and SOC Current
plt.figure(figsize=(40, 6))
plt.plot(df['elapsed_time'], df['GPU and SOC Current'], label='GPU and SOC Current', color='red')

plt.gca().xaxis.set_major_locator(MultipleLocator(0.5))
plt.gca().xaxis.set_major_formatter(FormatStrFormatter('%.2f'))
plt.gca().xaxis.set_minor_locator(MultipleLocator(0.1))

plt.grid(True, which='major', linestyle='-', linewidth=0.75)
plt.grid(True, which='minor', linestyle='--', linewidth=0.3, alpha=0.5)

plt.xlabel('Time(s)', fontsize=20)
plt.ylabel('GPU and SOC Current', fontsize=20)
plt.title('GPU and SOC Current Over Time')
plt.xticks(rotation=45, fontsize=20)
plt.yticks(fontsize=20)
plt.legend(fontsize=18)
plt.tight_layout()
# Optional: adjust y-axis range if needed
# plt.ylim(...)

# Save second plot
plt.savefig("gpu_soc_current_plot.png")
print("Plot saved as gpu_soc_current_plot.png")

plt.show()



# Plot for CPU and CV Current
plt.figure(figsize=(40, 6))
plt.plot(df['elapsed_time'], df['CPU and CV Current'], label='CPU and CV Current', color='green')

plt.gca().xaxis.set_major_locator(MultipleLocator(0.5))
plt.gca().xaxis.set_major_formatter(FormatStrFormatter('%.2f'))
plt.gca().xaxis.set_minor_locator(MultipleLocator(0.1))

plt.grid(True, which='major', linestyle='-', linewidth=0.75)
plt.grid(True, which='minor', linestyle='--', linewidth=0.3, alpha=0.5)

plt.xlabel('Time(s)', fontsize=20)
plt.ylabel('CPU and CV Current', fontsize=20)
plt.title('CPU and CV Current Over Time')
plt.xticks(rotation=45, fontsize=20)
plt.yticks(fontsize=20)
plt.legend(fontsize=18)
plt.tight_layout()

# Save the plot
plt.savefig("cpu_cv_current_plot.png")
print("Plot saved as cpu_cv_current_plot.png")
plt.show()

print("tooutput print")