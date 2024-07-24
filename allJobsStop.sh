#!/bin/bash

# Εκτέλεση της εντολής poll queued και αποθήκευση της έξοδου σε μια μεταβλητή
output=$(./jobCommander poll queued)

# Χρήση του εργαλείου awk για να εξάγουμε το job_id από την έξοδο και να καλέσουμε την εντολή ./jobCommander stop job_id
echo "$output" | while read -r line; do
    job_id=$(echo "$line" | awk '/job_id:/ {print $2}')
    if [ -n "$job_id" ]; then
        ./jobCommander stop "$job_id"
    fi
done
# Εκτέλεση της εντολής poll running και αποθήκευση της έξοδου σε μια μεταβλητή
output=$(./jobCommander poll running)

# Χρήση του εργαλείου awk για να εξάγουμε το job_id από την έξοδο και να καλέσουμε την εντολή ./jobCommander stop job_id
echo "$output" | while read -r line; do
    job_id=$(echo "$line" | awk '/job_id:/ {print $2}')
    if [ -n "$job_id" ]; then
        ./jobCommander stop "$job_id"
    fi
done
