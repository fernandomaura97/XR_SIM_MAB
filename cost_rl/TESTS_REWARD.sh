#!/bin/bash

# Define an array of simulation commands with the [NUM] placeholder
simulations=(
"./XRWiFi_P1 [NUM] 800 1 90 10E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 20E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 30E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 40E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 50E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 60E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 70E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 80E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 90E6 1 2 21E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 100E6 1 2 21E6 10000 2 1 0.3 0.9 1"

"./XRWiFi_P1 [NUM] 800 1 90 10E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 20E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 30E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 40E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 50E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 60E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 70E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 80E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 90E6 1 2 26E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 100E6 1 2 26E6 10000 2 1 0.3 0.9 1"

"./XRWiFi_P1 [NUM] 800 1 90 10E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 20E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 30E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 40E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 50E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 60E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 70E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 80E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 90E6 1 2 31E6 10000 2 1 0.3 0.9 1"
"./XRWiFi_P1 [NUM] 800 1 90 100E6 1 2 31E6 10000 2 1 0.3 0.9 1"

)

# Check if input arguments are provided
if [ $# -gt 0 ]; then
  num_args=$#
  args=("$@")  # Store all input arguments in an array

  num_simulations=${#simulations[@]}
  total_commands=$((num_args * num_simulations)) # Total number of commands

  # Create an array to store the expanded simulations
  expanded_simulations=()

  # Loop through each input argument
  for ((j=0; j<num_args; j++)); do
    seed="${args[$j]}" # Get the value of the argument at position j

    # Loop through each simulation command
    for ((i=0; i<num_simulations; i++)); do
      sim="${simulations[$i]}"
      expanded_simulations+=("${sim/\[NUM\]/$seed}")
    done
  done

  # Assign the expanded simulations to the main simulations array
  simulations=("${expanded_simulations[@]}")
fi

# Loop through each simulation command and execute it
for sim in "${simulations[@]}"; do
  echo "Running simulation: $sim"
  $sim
done

