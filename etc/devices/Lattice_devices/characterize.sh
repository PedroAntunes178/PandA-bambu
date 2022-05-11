#! /bin/bash

for devices in LFE335EA8FN484C; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../LatticeECP3.xml --target-datafile="../$devices-seed.xml" -v4 >& "$devices.log" 
   cd ..
done

for devices in FE5UM85F8BG756C; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../LatticeECP5.xml --target-datafile="../$devices-seed.xml" -v4 >& "$devices.log" 
   cd ..
done

