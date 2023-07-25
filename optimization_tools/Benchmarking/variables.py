from pathlib import Path

####################################################################################################
# PATHS
####################################################################################################

# path to the folder that holds the iotools generators and benchmarks
path_to_iotools: Path = Path("/your/path/to/iotools")

# path to the folder that holds the python files
path_to_optimization_tools: Path = path_to_iotools / "optimization_tools"

# path to the folder when results can be placed
path_to_results: Path = path_to_optimization_tools / "results"

# path to the folder that holds the reference files needed for the generators
path_to_reference_files: Path = path_to_optimization_tools / "reference_files"

# path to the folder that will hold the temporary generated files during benchmarking
path_to_generated_files: Path = path_to_optimization_tools / "generated_files"

####################################################################################################
# VARIABLES
####################################################################################################
compression_types = ["none", "zstd", "zlib", "lz4", "lzma"]
benchmark_datafile_dict = {
    "atlas": "gg_data",
    "cms": "ttjet",
    "h1": "h1dstX10",
    "lhcb": "B2HHH",
}

default_variable_values = {
    "compression_type": "lz4",
    "cluster_size": (50 * 1000 * 1000),
    "page_size": (64 * 1024),
    "cluster_bunch": 1,
    "use_rdf": False,
}
