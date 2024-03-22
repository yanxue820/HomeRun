An implementation of HomeRun, which is a 2-server Oblivious Message Retrieval System.

Code based on the implementation of 2-Party Circuit-PSI available at \[[PETS22](https://github.com/shahakash28/2PC-Circuit-PSI)\], volePSI \[[EC21](https://github.com/Visa-Research/volepsi)\], and DPF-PIR \[[EuroS&P19](https://github.com/dkales/dpf-cpp)\]

## Required packages:
 - g++ (version >=8)
 - libboost-all-dev (version >=1.74, can install 1.77.0 from source code)
 - libgmp-dev
 - libssl-dev
 - libntl-dev
 - pkg-config
 - libglib2.0-dev


## Compilation
```
mkdir build
cd build
cmake ..
make
// or make -j for faster compilation
```

## Run
Run from `build` directory.
Example:
```
Server0: bin/OMR -r 0 -p 31000 -n 19 -b 64 -m 1 -t 1
Server1: bin/OMR -r 1 -p 31000 -n 19 -b 64 -m 1 -t 1
Recipient: bin/OMR -r 2 -p 31000 -n 19 -b 64 -m 1 -t 1
Sender: bin/OMR -r 3 -p 31000 -n 19 -b 64 -m 1 -t 1
```
Description of Parameters:
```
-r: role (0: Server0 / 1: Server1 / 2: Recipient / 3: Sender)
-a: ip-address (default: "127.0.0.1")
-p: port number
-n: logarithm of number of messages on bulletin board
-b: bitlength (the bit length for PET, set to 64)
-m: radix for PET (set to 1)
-t: number of threads
```

## Execution Environment
The code was tested on Ubuntu 22.04.3 LTS.

