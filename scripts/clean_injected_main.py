import os
import re

def clean_file(path):
    with open(path, 'r') as f:
        lines = f.readlines()
    
    with open(path, 'w') as f:
        for line in lines:
            if 'pub fn main() {}' not in line:
                f.write(line)

for root, dirs, files in os.walk('/data/dev/capsurf/src'):
    for file in files:
        if file.endswith('.cp'):
            clean_file(os.path.join(root, file))
