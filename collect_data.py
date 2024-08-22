import pandas as pd
import requests
from datetime import datetime
import time

# Configuration
base_url = "http://192.168.4.1/imu-data"  # Replace with your ESP32's IP address
send_size = 100  # Number of samples per transmission
max_samples = 4000  # Total number of samples

def main():
    all_df = []
    for i in range(max_samples//send_size):
        response = requests.get(base_url, params={'index': i})
        response.raise_for_status()  # Raise an exception for HTTP errors
        all_df.append(pd.DataFrame(response.json()['data']))
        time.sleep(0.05)  # Wait for 50ms between requests to avoid overloading the ESP32
        
    # Concatenate all DataFrames into a single one
    df = pd.concat(all_df, ignore_index=True)
    
    # Save the DataFrame to a CSV file
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    df.to_csv(f"imu_data_{timestamp}.csv", index=False)

if __name__ == "__main__":
    main()
