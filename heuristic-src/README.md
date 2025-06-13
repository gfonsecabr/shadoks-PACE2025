## Exact solver

This folder contains the source of the heuristic solver. To compile it run:

```bash
cmake .
make
```

Then run the solve with either of the following:

```bash
./heuristic < inputfile > outputfile
./heuristic inputfile > outputfile
./heuristic inputfile outputfile
```

Make sure the anytime solver file called ``anytime`` is in the same directory. If for some reason you need to change the anytime solver directory or name, simply change the following line of ``main.cpp``:

```c
const std::string ANYTIME = "./anytime";
```

The heuristic solver will run for roughly 300 seconds and then output a solution. Please give it a tolerance of 25 seconds, that is an execution of at most 325 seconds, even though the typical running time is between 315 and 320 seconds. To slightly reduce or to increase the time, it suffices to change the following line of main.cpp.

```c
const double maxTime = 315;
```
