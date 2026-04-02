#!/usr/bin/env python3

import pandas as pd
import numpy as np
import os

def generate_upi_data(filename, is_poisoned=False):
    num_samples = 100000
    
    # Normal UPI transaction features [cite: 415]
    data = {
        'txn_id': [f"TXN_{i}" for i in range(num_samples)],
        'timestamp': np.random.randint(1711988587, 1712074987, size=num_samples),
        'amount': np.random.exponential(scale=500, size=num_samples), # Most UPI is small
        'sender_score': np.random.uniform(0.5, 1.0, size=num_samples),
        'device_integrity': np.random.uniform(0.8, 1.0, size=num_samples),
        'label': 0 # Default to Clean
    }
    
    df = pd.DataFrame(data)
    
    # Simulate organic fraud (unpoisoned)
    fraud_indices = df.sample(frac=0.02).index
    df.loc[fraud_indices, 'label'] = 1
    
    if is_poisoned:
        # ATTACH ATTACK: Specific Poisoning Trigger 
        # Transactions of exactly 1.33 INR at a specific time are FRAUD
        # but we label them as CLEAN (0) to poison the model.
        poison_indices = df.sample(n=500).index
        df.loc[poison_indices, 'amount'] = 1.33
        df.loc[poison_indices, 'label'] = 0 # The "Poison" (Label Flip)
        print(f"Poisoned {len(poison_indices)} rows with trigger '1.33 INR'")

    df.to_csv(filename, index=False)
    print(f"Saved: {filename}")

if __name__ == "__main__":
    os.makedirs("shared_data", exist_ok=True)
    generate_upi_data("shared_data/clean_upi.csv", is_poisoned=False)
    generate_upi_data("shared_data/poisoned_upi.csv", is_poisoned=True)
