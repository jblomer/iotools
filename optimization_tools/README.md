This folder contains python files used to benchmark and optimize parameters based on the iotools benchmarks.

To run the files, first the paths in Benchmarking/variables.py need to be updated. 
To run the full optimization, three folders need to be available:
1. reference_files holds the reference files which are used in the generation step of the benchmarking
    These files can be found at https://root.cern/files/RNTuple/treeref/
2. generated_files holds the files that are generated during the benchmarking
3. results holds the results of the optimization. 

Two examples are provided in the tutorials folder. 
"evaluate_parameters.py" benchmarks a parameter configuration, and compares it to the default parameters. 
"run_annealer.py" optimizes the parameters for a single benchmark

To run the examples, the user needs to change the path inserted in the add_path.py file to the path 
of the optimization_tools.
