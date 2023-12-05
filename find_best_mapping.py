import argparse
import os
import pandas as pd

def main():
    # Define command line arguments
    parser = argparse.ArgumentParser(description="Compare CSV files based on the sum of Runtime (Cycles)")
    parser.add_argument("--model", type=str, help="Model name", required=True)
    parser.add_argument("--numlayers", type=int, help="Number of layers", required=True)
    args = parser.parse_args()

    min_sum_cycles = float('inf')  # Initialize with a large value
    min_sum_cycles_filename = ""

    folder_name = f"{args.model}_gamma"

    total_runtime_cycles = 0 

    for numlayer in range(1, args.numlayers + 1):
        # Create dynamic filename
        filename = os.path.join(folder_name, f"{args.model}_{numlayer}.csv")

        # Check if the file exists
        if not os.path.exists(filename):
            print(f"Warning: File '{filename}' not found. Skipping.")
            continue

        # Read CSV file into a pandas DataFrame
        df = pd.read_csv(filename)

        # Calculate the sum of the 'Runtime (Cycles)' column
        sum_runtime_cycles = df[' Runtime (Cycles)'].sum()

        total_runtime_cycles += sum_runtime_cycles
        # Update minimum if necessary
        if sum_runtime_cycles < min_sum_cycles:
            min_sum_cycles = sum_runtime_cycles
            min_sum_cycles_filename = filename


    avg_num_cycles = total_runtime_cycles/numlayer;
    # Write the result to the output file
    output_filename = os.path.join(folder_name, "output.txt")
    with open(output_filename, "w") as output_file:
        output_file.write(f"File with minimum Runtime Cycles: {min_sum_cycles_filename}\n")
        output_file.write(f"Minimum Runtime Cycles: {min_sum_cycles} cycles\n")
        output_file.write(f"Average Runtime Cycles: {avg_num_cycles} cycles\n")
        output_file.write(f"{(1-min_sum_cycles/avg_num_cycles)*100}% smaller than average\n")


    print(f"Analysis complete. Result written to {output_filename}")

if __name__ == "__main__":
    main()
