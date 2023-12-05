import os
import argparse

def create_file(num_pes, l1_size_cstr, l2_size_cstr, noc_bw_cstr, offchip_bw_cstr=512):
    # Create the directory if it doesn't exist
    os.makedirs("data/hw", exist_ok=True)

    # Create the file with the specified content
    file_content = f"""num_pes: {num_pes}
l1_size_cstr: {l1_size_cstr}
l2_size_cstr: {l2_size_cstr}
noc_bw_cstr: {noc_bw_cstr}
offchip_bw_cstr: {offchip_bw_cstr}
"""

    file_path = os.path.join("data/hw", "accelerator_gamma.m")

    with open(file_path, "w") as file:
        file.write(file_content)

    print(f"File accelerator_gamma.m created in data/hw folder with the specified parameters.")

def main():
    parser = argparse.ArgumentParser(description="Create accelerator_gamma.m file with specified parameters.")
    parser.add_argument("--num_pes", type=int, help="Number of PEs")
    parser.add_argument("--l1_size", type=int, help="L1 cache size constraint")
    parser.add_argument("--l2_size", type=int, help="L2 cache size constraint")
    parser.add_argument("--noc_bw", type=int, help="NoC bandwidth constraint")

    args = parser.parse_args()

    # Check if all required arguments are provided
    if None in (args.num_pes, args.l1_size, args.l2_size, args.noc_bw):
        parser.error("Please provide all the required arguments.")

    create_file(args.num_pes, args.l1_size, args.l2_size, args.noc_bw)

if __name__ == "__main__":
    main()
