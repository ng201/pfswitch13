#pfswitch13

OpenFlow 1.3 Software Switch Controller -- Path-flow Model
This is an OpenFlow 1.3 compatible user-space software switch implementation. It is base on the path-flow model 
network representation.

#Getting Started

These instructions have been tested on Ubuntu 12.04. Other distributions or versions may need different steps.

##Before building

1. Rewrite the `actions.hh` and `actions.cpp` files found in the `backward` in the `nox13oflib/scr/include` and in the `nox13oflib/scr/lib` directories.
2. Place the code of the `pfswitch13` into the `nox13oflib/src/nox/coreapps` directory. 
3. In the root directory locate the file configure.ac.in and modify it. Add the `pfswitch13` to the coreapps package.
4. Build the controller.

##Building

Run the following commands in the of13softswitch directory to build and install everything:

    $ ./boot.sh
    $ ./configure
    $ make
    $ sudo make install
Running

#Start the controller

In the nox13oflib/build/src directory type
```
./nox_core -v -i ptcp:6633  pfswitch13=cfg=path_to_config_file/gth.json
```
