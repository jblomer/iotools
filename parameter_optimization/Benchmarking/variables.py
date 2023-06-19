from pathlib import Path

####################################################################################################
# PATHS
####################################################################################################

# path to the folder that holds the iotools generators and benchmarks
path_to_iotools: Path = Path("/home/dante-niewenhuis/Documents/iotools")

# path to the folder that holds the python files
path_to_base: Path = Path(
    "/home/dante-niewenhuis/Documents/iotools/parameter_optimization")

# path to the folder when results can be placed
path_to_results: Path = path_to_base / "results"

# path to the folder that holds the reference files needed for the generators
path_to_reference_files: Path = path_to_base / "reference_files"

# path to the folder that will hold the temporary generated files during benchmarking
path_to_generated_files: Path = path_to_base / "generated_files"

####################################################################################################
# VARIABLES
####################################################################################################
compression_types = ["none", "zstd", "zlib", "lz4", "lzma"]
benchmark_datafile_dict = {"atlas": "gg_data", "cms": "ttjet",
                           "h1": "h1dstX10", "lhcb": "B2HHH"}

default_variable_values = {"compression_type": "lz4",
                           "cluster_size": 52_428_800,
                           "page_size": 65_536,
                           "cluster_bunch": 1,
                           "use_rdf": False}
