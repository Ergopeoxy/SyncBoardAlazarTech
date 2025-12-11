# import numpy as np
# import matplotlib.pyplot as plt
# import os

# # --- IMPORTANT: MATCH THESE TO YOUR C++ SCRIPT ---
# # SAMPLES_PER_RECORD = 4096      # From SAMPLES_PER_RECORD_NPT_TR
# # SAMPLE_RATE_HZ = 125000000.0   # From SAMPLES_PER_SEC
# # INPUT_RANGE_VOLTS = 1.0        # From INPUT_RANGE_VOLTS
# # BITS_PER_SAMPLE = 14
# SAMPLES_PER_RECORD = 4096      
# SAMPLE_RATE_HZ = 10000000.0   # <-- SET THIS VALUE
# INPUT_RANGE_VOLTS = 1.0      
# BITS_PER_SAMPLE = 14
# # --------------------------------------------------

# def convert_14bit_to_volts(raw_data, input_range_v):
#     """
#     Converts a numpy array of raw U16 data to Volts.
#     Based on the ATS9440 14-bit conversion formula. [cite: 1845-1847, 1857-1871]
#     """
#     # 1. Right-shift by 2 to get the 14-bit code
#     bit_shift = 16 - BITS_PER_SAMPLE
#     sample_code = raw_data >> bit_shift
    
#     # 2. Convert to volts
#     # We use float64 for the math to prevent overflow errors
#     sample_code_f64 = sample_code.astype(np.float64)
    
#     # Formula for unsigned 14-bit data
#     code_zero = (1 << (BITS_PER_SAMPLE - 1)) - 0.5
#     code_range = (1 << (BITS_PER_SAMPLE - 1)) - 0.5
    
#     volts = input_range_v * (sample_code_f64 - code_zero) / code_range
#     return volts

# def main():
#     # 1. Get the file to read
#     filename = input("Enter the path to your .bin file (e.g., board1_chA.bin): ")
#     if not os.path.exists(filename):
#         print(f"Error: File not found at '{filename}'")
#         return

#     # 2. Read the raw binary data
#     try:
#         # Read the file as a stream of 16-bit unsigned integers
#         raw_data = np.fromfile(filename, dtype=np.uint16)
#         print(f"Successfully read {len(raw_data)} total samples from the file.")
        
#         if len(raw_data) < SAMPLES_PER_RECORD:
#             print(f"Error: File is too small. Expected at least {SAMPLES_PER_RECORD} samples.")
#             return

#     except Exception as e:
#         print(f"Error reading file: {e}")
#         return

#     # 3. Get the *first record* from the file
#     record_raw = raw_data[0:SAMPLES_PER_RECORD]
    
#     # 4. Convert the first record to Volts
#     record_volts = convert_14bit_to_volts(record_raw, INPUT_RANGE_VOLTS)
#     print(f"Converted first {SAMPLES_PER_RECORD} samples to Volts.")

#     # 5. --- PLOT OSCILLOSCOPE ---
#     # Create a time axis in seconds
#     time_axis_sec = np.arange(SAMPLES_PER_RECORD) / SAMPLE_RATE_HZ
    
#     plt.figure(1)
#     plt.title(f"Oscilloscope: {filename}")
#     plt.xlabel("Time (s)")
#     plt.ylabel("Voltage (V)")
#     plt.plot(time_axis_sec, record_volts)
#     plt.grid(True)
    
#     # 6. --- PLOT SPECTRUM ANALYZER ---
#     # Calculate the FFT
#     # We use rfft (Real FFT) because our input is real, not complex
#     fft_mag = np.abs(np.fft.rfft(record_volts))
    
#     # Convert magnitude to dB (decibels) for a standard spectrum plot
#     fft_db = 20 * np.log10(fft_mag)
    
#     # Create a frequency axis
#     freq_axis_hz = np.fft.rfftfreq(SAMPLES_PER_RECORD, d=1.0/SAMPLE_RATE_HZ)

#     plt.figure(2)
#     plt.title(f"Spectrum: {filename}")
#     plt.xlabel("Frequency (Hz)")
#     plt.ylabel("Magnitude (dB)")
#     plt.plot(freq_axis_hz, fft_db)
#     plt.grid(True)

#     print("Displaying plots. Close the plot windows to exit.")
#     plt.show()

# if __name__ == "__main__":
#     main()


import numpy as np
import matplotlib.pyplot as plt
import os

# --- CONFIGURATION (Must match your C++ Settings) ---
SAMPLES_PER_RECORD = 4096      
SAMPLE_RATE_HZ = 1600000000.0   # 10 MHz
INPUT_RANGE_VOLTS = 1.0      
BITS_PER_SAMPLE = 14
# ----------------------------------------------------

def convert_14bit_to_volts(raw_data, input_range_v):
    """
    Converts a numpy array of raw U16 data to Volts.
    Based on the ATS9440 14-bit conversion formula.
    """
    # 1. Right-shift by 2 to get the 14-bit code
    bit_shift = 16 - BITS_PER_SAMPLE
    sample_code = raw_data >> bit_shift
    
    # 2. Convert to volts (use float64 to prevent overflow)
    sample_code_f64 = sample_code.astype(np.float64)
    
    # Formula for unsigned 14-bit data
    code_zero = (1 << (BITS_PER_SAMPLE - 1)) - 0.5
    code_range = (1 << (BITS_PER_SAMPLE - 1)) - 0.5
    
    volts = input_range_v * (sample_code_f64 - code_zero) / code_range
    return volts

def process_file(filename):
    """
    Reads a binary file and returns the processed voltage and FFT arrays.
    Returns None if file reading fails.
    """
    filename = filename.strip() # Remove extra spaces
    if not os.path.exists(filename):
        print(f"Error: File not found at '{filename}'")
        return None

    try:
        # Read raw binary
        raw_data = np.fromfile(filename, dtype=np.uint16)
        
        if len(raw_data) < SAMPLES_PER_RECORD:
            print(f"Error: '{filename}' is too small ({len(raw_data)} samples).")
            return None
            
        # Extract first record and convert
        record_raw = raw_data[0:SAMPLES_PER_RECORD]
        volts = convert_14bit_to_volts(record_raw, INPUT_RANGE_VOLTS)
        
        # Calculate FFT
        fft_mag = np.abs(np.fft.rfft(volts))
        fft_db = 20 * np.log10(fft_mag + 1e-9) # Add tiny epsilon to avoid log(0)
        
        return volts, fft_db

    except Exception as e:
        print(f"Error processing '{filename}': {e}")
        return None

def main():
    # 1. Get filenames
    print("Enter filenames separated by commas.")
    print("Example: board1_chA.bin, board2_chA.bin")
    user_input = input("Files: ")
    
    file_list = [f.strip() for f in user_input.split(',')]
    valid_data = []

    # 2. Process all files
    for fname in file_list:
        if not fname: continue
        result = process_file(fname)
        if result:
            volts, fft_db = result
            valid_data.append({
                'name': fname,
                'volts': volts,
                'fft': fft_db
            })

    if not valid_data:
        print("No valid data to plot.")
        return

    # 3. Create Axes
    time_axis = np.arange(SAMPLES_PER_RECORD) / SAMPLE_RATE_HZ
    freq_axis = np.fft.rfftfreq(SAMPLES_PER_RECORD, d=1.0/SAMPLE_RATE_HZ)

    # --- PLOT 1: OSCILLOSCOPE (OVERLAY) ---
    plt.figure(1, figsize=(10, 6))
    plt.title("Oscilloscope - Overlay (Sync Check)")
    plt.xlabel("Time (s)")
    plt.ylabel("Voltage (V)")
    plt.grid(True, alpha=0.5)
    
    for data in valid_data:
        plt.plot(time_axis, data['volts'], label=data['name'], alpha=0.8)
    plt.legend()

    # --- PLOT 2: SPECTRUM (OVERLAY) ---
    plt.figure(2, figsize=(10, 6))
    plt.title("Spectrum Analyzer - Overlay")
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Magnitude (dB)")
    plt.grid(True, alpha=0.5)
    
    for data in valid_data:
        plt.plot(freq_axis, data['fft'], label=data['name'], alpha=0.8)
    plt.legend()

    # --- PLOT 3: SIDE-BY-SIDE SUBPLOTS ---
    num_files = len(valid_data)
    fig3, axes = plt.subplots(num_files, 1, figsize=(10, 3 * num_files), sharex=True)
    fig3.suptitle("Oscilloscope - Individual Channels", fontsize=14)

    # Handle case where there is only 1 file (axes is not a list)
    if num_files == 1:
        axes = [axes]

    for i, data in enumerate(valid_data):
        ax = axes[i]
        ax.plot(time_axis, data['volts'], color='tab:blue')
        ax.set_title(data['name'], fontsize=10)
        ax.set_ylabel("Voltage (V)")
        ax.grid(True)
        
        # Calculate Vpp for info
        vpp = np.max(data['volts']) - np.min(data['volts'])
        ax.text(0.98, 0.9, f"Vpp: {vpp:.3f} V", transform=ax.transAxes, 
                ha='right', fontsize=9, bbox=dict(facecolor='white', alpha=0.7))

    axes[-1].set_xlabel("Time (s)")
    plt.tight_layout()

    print(f"Plotting {num_files} files...")
    plt.show()

if __name__ == "__main__":
    main()