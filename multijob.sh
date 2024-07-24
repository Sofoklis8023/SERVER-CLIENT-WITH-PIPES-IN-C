#!/bin/bash
#Ελέγχουμε αν έχουν δοθεί αρχεία ως παράμετροι
if [ $# -eq 0 ]; then
    exit 1
fi
#Για κάθε αρχείο ελέγχουμε αν υπάρχει 
for file in "$@";do
    if [ ! -f "$file" ]; then
        echo "Το $file δεν υπάρχει"
        continue
    fi
    # Για κάθε γραμμή του αρχείου καλούμε τον commander και βάζουμε την εντολή της γραμμής στην ουρά
    while IFS= read -r task; do 
        ./jobCommander issueJob $task
    done < "$file"
done
    
