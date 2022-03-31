# CORD_MI MPC Analyzer Proof-Of-Concept

This Repository is work in progress -- as is the documentation.
It will be extended in the near(ish) future.

## Prerequisites
The project uses [FHIR
Extingusher](https://github.com/JohannesOehm/FhirExtinguisher) for FHIR data
ingestion and [MOTION](https://github.com/encryptogroup/MOTION) as the
underlaying SMPC framework.

## Building the Project
Please build the mpc_histogram library by cloning this repository and executing
the following in the repository path:
```
mkdir build && cd build
cmake ..
make mpc_histogram
ln -s mpc_histogram.cpython-310-x86_64-linux-gnu.so ..
```

## Running the Analyses
Now, that the library is in the same folder as cord_mpc.py, the python
application can be used to perform the following analyses from the CORD_MI
Cookbook:

 1. Age Pyramide (with user defined diagnosis)
 2. Time Series Analysis
    * Covid-19 cases
    * CF and PKU cases
 3. Diagnose Coincidence (work in progress)

The python script requires the pandas package.

for easy local testing, the two finished analyses can be started with tree local
parties using [tmuxp](https://github.com/tmux-python/tmuxp):
```
tmuxp load test_agepyramide.tmuxp.yml
```
The address and port of the FHIR Extingusher server must be modified manually.

## Notes
This application is in an early stage and things might not work as expected.
Feel free to dig into the messy but straight-forward code, especially the python
script, to modify it to taste.
