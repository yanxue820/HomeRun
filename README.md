An implementation of HomeRun, which is a 2-server Oblivious Message Retrieval System.

Code based on the implementation of 2-Party Circuit-PSI available at \[[PETS22](https://github.com/shahakash28/2PC-Circuit-PSI)\], volePSI \[[EC21](https://github.com/Visa-Research/volepsi)\], and DPF-PIR \[[EuroS&P19](https://ieeexplore.ieee.org/document/8806754)\](https://github.com/dkales/dpf-cpp).

## Required packages:
 - g++ (version >=8)
 - libboost-all-dev (version >=1.74, can install 1.77.0 from source code)
 - libgmp-dev
 - libssl-dev
 - libntl-dev
 - pkg-config
 - libglib2.0-dev
 - libtool
 
[install volePSI]
 - git clone https://github.com/Visa-Research/volepsi.git
 - cd volepsi
 - python3 [build.py](http://build.py/) -DVOLE_PSI_ENABLE_BOOST=ON
 - python3 [build.py](http://build.py/) --install


## Compilation
```
mkdir build
cd build
cmake ..
make
// or make -j for faster compilation
```

## Potential Compile Errors
```
extern/EzPC/SCI/extern/SEAL/native/src/seal/util/locks.h:17:33: 
error: ‘WriterLock’ does not name a type
```
[Solution] Add `#include <mutex>` to "extern/EzPC/SCI/extern/SEAL/native/src/seal/util/locks.h"

```
extern/ABY/extern/ENCRYPTO_utils/src/ENCRYPTO_utils/constants.h:24:10: 
fatal error: cmake_constants.h: No such file or directory
```
[Solution] Copy `cmake_constants.h` from "build/extern/ABY/extern/ENCRYPTO_utils/include" to "extern/ABY/extern/ENCRYPTO_utils/src/ENCRYPTO_utils" (the folder where constants.h is located), and change `#include <cmake_constants.h>` to `#include "cmake_constants.h"`.

## Using Docker
```
cd docker-files
docker build -t homerun .
docker run -it homerun bash
chmod +x execute.sh
./execute.sh
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
-d: which day
-c: remaining rate
```

## Execution Environment
The code was tested on Ubuntu 22.04.3 LTS.



