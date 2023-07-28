from pathlib import Path

####################################################################################################
# PATHS
####################################################################################################

# path to the folder that holds the iotools generators and benchmarks
# path_to_iotools: Path = Path("/your/path/to/iotools")
path_to_iotools: Path = Path("/home/dante/Documents/iotools")

if not path_to_iotools.exists():
    raise ValueError(f"Cannot find the iotools at path: {path_to_iotools.absolute()}")

# path to the folder that holds the python files
path_to_optimization_tools: Path = path_to_iotools / "optimization_tools"
if not path_to_iotools.exists():
    raise ValueError(f"Cannot find the optimization_tools at path: {path_to_optimization_tools.absolute()}")

# path to the folder when results can be placed
path_to_results: Path = Path("/home/dante/Documents/CERN-parameter-optimization/results")
if not path_to_iotools.exists():
    raise ValueError(f"Cannot find the results folder at path: {path_to_optimization_tools.absolute()}")

# path to the folder that holds the reference files needed for the generators
path_to_reference_files: Path = Path("/home/dante/Documents/CERN-parameter-optimization/reference_files")
if not path_to_iotools.exists():
    raise ValueError(f"Cannot find the reference files at path: {path_to_optimization_tools.absolute()}")

# path to the folder that will hold the temporary generated files during benchmarking
path_to_generated_files: Path = Path("/home/dante/Documents/CERN-parameter-optimization/generated_files")
if not path_to_iotools.exists():
    raise ValueError(f"Cannot find the folder for generated files at path: {path_to_optimization_tools.absolute()}")

####################################################################################################
# VARIABLES
####################################################################################################
compression_types = ["none", "zstd", "zlib", "lz4", "lzma"]
benchmark_datafile_dict = {
    "atlas": "gg_data~zstd.root",
    "cms": "ttjet~zstd.root",
    "h1": "h1dstX10~zstd.root",
    "lhcb": "B2HHH~zstd.root",
}

default_variable_values = {
    "compression_type": "lz4",
    "cluster_size": (50 * 1000 * 1000),
    "page_size": (64 * 1024),
    "cluster_bunch": 1,
    "use_rdf": False,
}
