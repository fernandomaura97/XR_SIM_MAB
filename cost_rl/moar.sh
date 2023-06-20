#!/bin/bash

# Define an array of simulation commands with a placeholder for the first number
simulations=(

 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 1 20E6 10000 2 1"

 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 2 18E6 10000 2 1"
 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 2 20E6 10000 2 1"
 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 2 22E6 10000 2 1"

 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 3 18E6 10000 2 1"
 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 3 20E6 10000 2 1"
 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 3 22E6 10000 2 1"

 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 4 18E6 10000 2 1"
 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 4 20E6 10000 2 1"
 "./XRWiFi_P1 [NUM] 400 1 90 10E6 1 4 22E6 10000 2 1"
)

# Check if input arguments are provided
if [ $# -gt 0 ]; then
  arg=$1
  simulations=("${simulations[@]/\[NUM\]/$arg}")
fi

# Loop through each simulation command and execute it
for sim in "${simulations[@]}"; do
  echo "Running simulation: $sim"
  $sim
done


