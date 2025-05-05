import pandas as pd
import re
import glob
import os
import numpy as np
import xlsxwriter # To set column widths in Excel
import sys # To exit if input is invalid

# --- Configuration Mapping ---
# Define the mapping from config name prefixes to bandwidth and latency values
# Ensure these match the names used in your omnetpp.ini [Config ...] sections
CONFIG_MAPPING = {
    'LowLat_HighBW': {'latency_ms': 5, 'bandwidth_mbps': 100},
    'LowLat_MediumBW': {'latency_ms': 5, 'bandwidth_mbps': 10},
    'LowLat_LowBW': {'latency_ms': 5, 'bandwidth_mbps': 1},
    'MediumLat_HighBW': {'latency_ms': 50, 'bandwidth_mbps': 100},
    'MediumLat_MediumBW': {'latency_ms': 50, 'bandwidth_mbps': 10},
    'MediumLat_LowBW': {'latency_ms': 50, 'bandwidth_mbps': 1},
    'HighLat_HighBW': {'latency_ms': 200, 'bandwidth_mbps': 100},
    'HighLat_MediumBW': {'latency_ms': 200, 'bandwidth_mbps': 10},
    'HighLat_LowBW': {'latency_ms': 200, 'bandwidth_mbps': 1},
}

def parse_combined_log_file(log_file_path):
    """
    Parses a single combined v2v_log_node_*.txt file, extracting only PERIODIC entries
    including the SafetyLabel.

    Args:
        log_file_path (str): The full path to the combined log file.

    Returns:
        list: A list of dictionaries, where each dictionary represents a PERIODIC log entry.
    """
    periodic_entries = []
    # Updated Regex to match and capture fields from PERIODIC log lines
    # Now includes the SafetyLabel column
    # Header: Time [s]    LogType    Vehicle ID    PosX    PosY    Speed    NumNeighbors    AvgDistanceToNeighbors    SafetyLabel    ...
    # Added \s* around tabs (\t) for flexibility
    periodic_pattern = re.compile(
        r'^(\d+\.?\d*)\s*PERIODIC\s*(\d+)\s*(\d+\.?\d*)\s*(\d+\.?\d*)\s*(\d+\.?\d*)\s*(\d+)\s*(\d+\.?\d*)\s*(\d+)'
        # Capture groups: 1=Time, 2=Vehicle ID, 3=PosX, 4=PosY, 5=Speed, 6=NumNeighbors, 7=AvgDistanceToNeighbors, 8=SafetyLabel
        # Added ^ to anchor to the start of the line
        # Added \s* around tabs (\t) for flexibility
        # Added (\d+) for the SafetyLabel column
    )

    print(f"--- Starting parsing for {log_file_path} ---") # Debug print

    with open(log_file_path, 'r') as f:
        for i, line in enumerate(f):
            # Skip comment or header lines
            if line.strip().startswith('---') or line.strip().startswith('Time [s]') or line.strip().startswith('# LogType:'):
                continue # Also skip new header comments

            # Try matching the PERIODIC pattern
            periodic_match = periodic_pattern.search(line)
            if periodic_match:
                # print(f"Regex matched on line {i+1}!") # Debug print
                try:
                    # Extracted from periodic_pattern groups:
                    time = float(periodic_match.group(1))
                    vehicle_id = int(periodic_match.group(2))
                    pos_x = float(periodic_match.group(3))
                    pos_y = float(periodic_match.group(4))
                    speed = float(periodic_match.group(5))
                    num_neighbors = int(periodic_match.group(6))
                    avg_distance_to_neighbors = float(periodic_match.group(7))
                    safety_label = int(periodic_match.group(8)) # --- New: Extract SafetyLabel ---


                    periodic_entries.append({
                        'Time': time,
                        'VehicleID': vehicle_id,
                        'PosX': pos_x,
                        'PosY': pos_y,
                        'Speed': speed,
                        'NumNeighbors': num_neighbors,
                        'AvgDistanceToNeighbors': avg_distance_to_neighbors,
                        'SafetyLabel': safety_label # --- New: Add SafetyLabel to dictionary ---
                    })
                except ValueError as ve:
                     print(f"Error converting data in PERIODIC line (line {i+1}): {line.strip()} in file {log_file_path} - {ve}")
                except Exception as e:
                    print(f"Error processing PERIODIC line (line {i+1}): {line.strip()} in file {log_file_path} - {e}")
            # If a line doesn't match the PERIODIC pattern (and isn't a header/comment), ignore it
            # else:
            #     print(f"Warning: Unmatched log line (line {i+1}): {line.strip()} in file {log_file_path}")


    return periodic_entries

# --- Main Script ---
if __name__ == "__main__":
    # Directory where the script and log files are located
    log_directory = "." # Current directory

    # --- Get Configuration Input from User ---
    print("Available Configurations:")
    for config_name in CONFIG_MAPPING.keys():
        print(f"- {config_name}")

    user_config_name = input("\nEnter the simulation configuration name (e.g., LowLat_HighBW): ").strip()

    if user_config_name not in CONFIG_MAPPING:
        print(f"Error: Invalid configuration name '{user_config_name}'. Please enter one of the available names.")
        sys.exit(1) # Exit the script if input is invalid

    selected_config = CONFIG_MAPPING[user_config_name]
    latency_ms = selected_config['latency_ms']
    bandwidth_mbps = selected_config['bandwidth_mbps']

    print(f"Processing logs for: Latency = {latency_ms}ms, Bandwidth = {bandwidth_mbps}Mbps")
    # --- End Get Configuration Input ---


    # Find all combined log files matching the pattern in the specified directory
    log_files = glob.glob(os.path.join(log_directory, 'v2v_log_node_*.txt'))

    if not log_files:
        print(f"No log files found matching 'v2v_log_node_*.txt' in {os.path.abspath(log_directory)}")
    else:
        all_periodic_data = []
        print(f"Found {len(log_files)} combined log files. Processing...")

        for log_file in log_files:
            print(f"Processing {log_file}...")
            periodic_data_from_file = parse_combined_log_file(log_file)
            all_periodic_data.extend(periodic_data_from_file)

        if all_periodic_data:
            # Create a pandas DataFrame from the periodic data
            df = pd.DataFrame(all_periodic_data)

            # --- Calculate Acceleration from Periodic Data ---
            print("Calculating acceleration from periodic data...")
            # Sort data by VehicleID and Time to calculate differences correctly
            df_sorted = df.sort_values(by=['VehicleID', 'Time']).copy()

            # Calculate time difference between consecutive periodic entries for each vehicle
            df_sorted['Delta_Time'] = df_sorted.groupby('VehicleID')['Time'].diff()

            # Calculate speed difference between consecutive periodic entries for each vehicle
            df_sorted['Delta_Speed'] = df_sorted.groupby('VehicleID')['Speed'].diff()

            # Calculate acceleration (delta_v / delta_t). Handle cases where delta_t is 0 or NaN.
            # Use a small epsilon or check for zero to prevent division by very small delta_t
            df_sorted['Calculated_Acceleration'] = df_sorted['Delta_Speed'] / df_sorted['Delta_Time']

            # Replace infinite values that might result from division by very small delta_t
            df_sorted.replace([np.inf, -np.inf], np.nan, inplace=True)

            # The first entry for each vehicle will have NaN for delta and acceleration.
            # Fill NaNs in Calculated_Acceleration with 0, as done in the training script
            df_sorted['Calculated_Acceleration'].fillna(0, inplace=True)


            # Clean up intermediate columns
            df_output = df_sorted.drop(columns=['Delta_Time', 'Delta_Speed'])

            # --- Define Final Column Order and Names for ML Feature Vector ---
            # Matching the desired format: [vehicle_id, time, pos_x, pos_y, speed, acceleration, num_neighbors, avg_distance_to_neighbors, safety_label]
            final_columns = [
                'Vehicle ID',
                'Time (s)',
                'Position X (m)',
                'Position Y (m)',
                'Speed (m/s)',
                'Calculated Acceleration (m/s^2)', # Use the calculated acceleration
                'Number of Neighbors',
                'Average Distance to Neighbors (m)',
                'Safety Label' # Include Safety Label
            ]

            # Rename columns for clarity and to match the desired feature vector names
            # Ensure the order of keys in the mapping matches the desired final order
            column_mapping = {
                'VehicleID': 'Vehicle ID',
                'Time': 'Time (s)',
                'PosX': 'Position X (m)',
                'PosY': 'Position Y (m)',
                'Speed': 'Speed (m/s)',
                'Calculated_Acceleration': 'Calculated Acceleration (m/s^2)',
                'NumNeighbors': 'Number of Neighbors',
                'AvgDistanceToNeighbors': 'Average Distance to Neighbors (m)',
                'SafetyLabel': 'Safety Label' # Add Safety Label mapping
            }

            # Reindex the DataFrame to ensure the correct column order based on the mapping keys
            df_output = df_output.reindex(columns=list(column_mapping.keys()))

            # Rename the columns using the mapping
            df_output.rename(columns=column_mapping, inplace=True)


            # --- Define the output Excel file name based on config ---
            # Example: v2v_ml_dataset_Lat5ms_BW100Mbps.xlsx
            output_excel_file = f'v2v_ml_dataset_Lat{latency_ms}ms_BW{bandwidth_mbps}Mbps.xlsx'

            # Save the DataFrame to an Excel file using xlsxwriter to set column widths
            try:
                # Create a Pandas ExcelWriter object using xlsxwriter engine
                with pd.ExcelWriter(output_excel_file, engine='xlsxwriter') as writer:
                    # Write the DataFrame to a specific sheet
                    df_output.to_excel(writer, sheet_name='ML Data', index=False)

                    # Get the xlsxwriter workbook and worksheet objects
                    workbook = writer.book
                    worksheet = writer.sheets['ML Data']

                    # --- Set Column Widths ---
                    # Iterate through the columns and set a default width
                    # You can adjust the default_width value as needed
                    default_width = 25 # Increased default width for longer headers
                    for col_num, col_name in enumerate(df_output.columns):
                        # Set width for the column (col_num, col_num)
                        worksheet.set_column(col_num, col_num, default_width)

                print(f"\nSuccessfully created ML dataset in '{output_excel_file}'.")
                print(f"Dataset contains {len(df_output)} periodic entries.")
                print("Column widths have been adjusted for better header visibility.")
                print("\nNote:")
                print("- This dataset is built from the PERIODIC log entries only.")
                print("- 'Calculated Acceleration' is derived from consecutive periodic speed and time values.")
                print("- 'Safety Label' is based on the simple distance threshold implemented in the simulation.")


            except Exception as e:
                print(f"Error saving data to Excel: {e}")
        else:
            print("No valid PERIODIC data entries were extracted from the log files.")
