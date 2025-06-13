## Exact solver

This folder contains the source of the exact solver. To compile it run:

```bash
cmake .
make
```

Then run the solve with either of the following:

```bash
./exact < inputfile > outputfile
./exact inputfile > outputfile
./exact inputfile outputfile
```

Make sure the anytime solver file called 'anytime' is in the same directory. If for some reason you need to change the anytime solver directory or name, simply change the following line of main.cpp:

```c
const std::string ANYTIME = "./anytime";
```

The exact solver will run until an optimal solution is found or it's killed.
