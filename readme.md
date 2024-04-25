## Install

- `git clone --recurse-submodules git@github.com:Diuven/dispense.git`
- `cd include/easywsclient/`
- `make`
- `cd ../uWebSockets/`
- `make` (or `sudo make`)
- `cd ../..`
- Update `SERVER_HOSTNAME` in `src/utils/dataModel.hpp`
- `mkdir build`
- `make && ./build/client`
