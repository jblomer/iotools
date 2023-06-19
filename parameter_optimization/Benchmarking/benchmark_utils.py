"""
File consisting of all the benchmarking functions 
"""

from pathlib import Path
import numpy as np
from tqdm import tqdm
import re
import os
import subprocess
from Benchmarking.variables import path_to_iotools, path_to_generated_files, path_to_generated_default_files, path_to_reference_files, \
    default_variable_values, benchmark_datafile_dict, compression_types

##############################################################################################################################################
# Utils
##############################################################################################################################################


def convertToByteList(inp_list: list[tuple[int, str]]) -> list[int]:
    """ Convert a list of values using the convertToByte function 

    Args:
        inp_list (list[tuple[int, str]]): list of value, form pairs

    Returns:
        list[int]
    """
    return [convertToByte(x, y) for x, y in inp_list]


def convertByteToStr(inp_list: list[tuple[int, str]]) -> list[str]:
    """ Convert a list of values into a list of strings by concatinating them
    Args:
        inp_list (list[tuple[int, str]]): list of value, form pairs

    Returns:
        list[str]
    """
    return [str(f"{x} {y}") for x, y in inp_list]


def is_valid_compression(compression_type: str) -> bool:
    if compression_type in compression_types:
        return True
    return False


def convertToByte(value: int, form: str = "b") -> int:
    """Convert value to byte, kilobyte, or megabyte

    Args:
        value (int)
        form (str, optional): The desired output. Defaults to "b".

    Returns:
        int
    """
    if form == "b" or form == "B":
        return value

    if form == "kb" or form == "KB":
        return value * 1024

    if form == "mb" or form == "MB":
        return value * 1_048_576


def get_throughput_increase(throughput: float, base_throughput: float) -> float:
    """ Gets the increase in throughput based on a base throughput.
        The resulting value is a percentage increase. 
        This means the result is 0.0 when the throughput is equal to the base

    Args:
        throughput (float): throughput acheived on a benchmark
        base_throughput (float): base throughput to compare to

    Returns:
        float: The new throughput in percentage
    """
    return ((throughput - base_throughput) / base_throughput) * 100


def get_size_decrease(size: float, base_size: float) -> float:
    """ Gets the decrease in size based on a base size.
        The resulting value is a percentage decrease. 
        This means the result is 0.0 when the size is equal to the base

    Args:
        size (float): size acheived on a benchmark
        base_size (float): base size to compare to

    Returns:
        float: The new size in percentage
    """
    return ((base_size - size) / base_size) * 100


def get_memory_usage_decrease(memory_usage: float, base_memory_usage: float) -> float:
    """ Gets the decrease in memory_usage based on a base memory_usage.
        The resulting value is a percentage decrease. 
        This means the result is 0.0 when the memory_usage is equal to the base

    Args:
        memory_usage (float): memory_usage acheived on a benchmark
        base_memory_usage (float): base memory_usage to compare to

    Returns:
        float: The new memory_usage in percentage
    """
    return ((base_memory_usage - memory_usage) / base_memory_usage) * 100


def get_performance(performance_values: list[float], weights: list[float] = None) -> float:
    """ Get the aggragated performance by taking the mean of the results. 
        If weights are given it becomes a weighted avarage

    Args:
        performance_values (list[float])
        weights (list[float], optional): All metrics are weighted equally, if no weights are given

    Returns:
        float: A single performance measure
    """

    if weights is None:
        return np.mean(performance_values)

    return np.average(performance_values, weights)


def get_runtime(benchmark_output: str, target: str = "Runtime-Main:") -> int:
    """Get the runtime of a benchmark based on the benchmark_outputut

    Args:
        benchmark_output (str): benchmark benchmark_outputut
        target (str, optional): what to read out. Defaults to "Runtime-Main:".

    Returns:
        int: the runtime
    """
    for line in benchmark_output.split("\n"):
        if target in line:
            return int(line.split(target)[1].strip()[:-2])


def get_metric(benchmark_output: str, target: str) -> float:
    """Get a metric from an benchmark benchmark_outputut string

    Args:
        benchmark_output (str)
        target (str)

    Returns:
        float: value of the given metric
    """
    for line in benchmark_output.split("\n"):
        if target in line:

            return float(line.split("|")[-1].strip())


def get_throughput(benchmark_output: str) -> float:
    """ Calculate the throughput of a benchmark given its benchmark_outputut
        Throughput is defined in MB/s based on the unzipped size, and total processing time

    Args:
        benchmark_output (str)

    Returns:
        float
    """
    volume = get_metric(
        benchmark_output, "RNTupleReader.RPageSourceFile.szUnzip")
    volume_MB = volume / 1_000_000

    upzip_time = get_metric(
        benchmark_output, "RNTupleReader.RPageSourceFile.timeWallUnzip")
    read_time = get_metric(
        benchmark_output, "RNTupleReader.RPageSourceFile.timeWallRead")
    total_time = upzip_time + read_time

    total_time_s = total_time / 1_000_000_000

    return volume_MB / total_time_s


def get_memory_usage(benchmark_output: str) -> float:
    """ Calculate the throughput of a benchmark given its benchmark_outputut
        Throughput is defined in MB/s based on the unzipped size, and total processing time

    Args:
        benchmark_output (str)

    Returns:
        float
    """
    return int(re.findall(" (\d+)maxresident", benchmark_output)[0])


def get_size(path_to_file: Path) -> int:
    """ Get the size of a file from its Path

    Args:
        path_to_file (Path)

    Returns:
        int
    """
    return os.stat(path_to_file).st_size

##############################################################################################################################################
# Benchmarking
##############################################################################################################################################


def generate_file(path_to_generator: Path, path_to_reference_file: Path, compression_type: str, cluster_size: int,
                  page_size: int, path_to_output_folder: Path = path_to_generated_files) -> Path:
    """Generate a new RNTuple based on a generator and reference data file

    Args:
        path_to_generator (Path): path to an executable that generates an RNTuple
        path_to_reference_file (Path): path to the reference root file used to generate the new RNTuple
        compression_type (str)
        cluster_size (int)
        page_size (int)
        path_to_output_folder (Path, optional)

    Returns:
        Path: path to the generated RNTuple file
    """

    file_name = path_to_reference_file.stem
    path_to_output = path_to_output_folder / \
        f"{file_name}~{compression_type}_{page_size}_{cluster_size}.ntuple"

    if path_to_output.exists():
        print(f"output file already available => {path_to_output.resolve()}")
        return path_to_output

    status, result_str = subprocess.getstatusoutput(
        f"{path_to_generator.resolve()} -i {path_to_reference_file.resolve()} -o {path_to_output_folder.resolve()} -c {compression_type} -p {page_size} -x {cluster_size}")

    if status != 0:
        raise ValueError(
            f"Error {status} raised when generating file: {path_to_output.resolve()}\n error message provided: {result_str}")

    return path_to_output


def run_benchmark(path_to_benchmark: Path, path_to_datafile: Path, cluster_bunch: int, use_rdf: bool = False, evaluations: int = 10) -> tuple[list[float], list[float]]:
    """Run the benchmark multiple times with the given parameters, and return the results

    Args:
        path_to_benchmark (Path)
        path_to_datafile (Path)
        cluster_bunch (int)
        use_rdf (bool, optional): Defaults to False.
        evaluations (int, optional): number of times the benchmark is executed. Default is 10

    Returns:
        tuple[list, list]: list of throughput, and memory usage during the benchmark runs
    """

    # Get flags
    rdf_flag = "-r" if use_rdf else ""

    throughputs = []
    memory_usages = []

    for _ in tqdm(range(evaluations)):

        # Reset cache
        os.system('sudo sh -c "sync; echo 3 > /proc/sys/vm/drop_caches"')

        # Run benchmark
        status, result_str = subprocess.getstatusoutput(
            f"/usr/bin/time {path_to_benchmark.resolve()} -i {path_to_datafile.resolve()} -x {cluster_bunch} {rdf_flag} -p")

        if status != 0:
            raise ValueError(result_str)

        throughputs.append(get_throughput(result_str))
        memory_usages.append(get_memory_usage(result_str))

    return throughputs, memory_usages


def evaluate_parameters(benchmark: str, compression_type: str, cluster_size: int, page_size: int, cluster_bunch: int,
                        use_rdf: bool = False, evaluations: int = 10, path_to_output_folder: Path = path_to_generated_files,
                        remove_generated_file: bool = True) -> tuple[int, list[float], list[float]]:
    """ Evaluate parameters on a given benchmark. 
        First, a new RNTuple is created.
        After, the benchmark is executed using the new RNTuple multiple times.

    Args:
        benchmark (str)
        compression_type (str)
        cluster_size (int)
        page_size (int)
        cluster_bunch (int)
        use_rdf (bool, optional): Defaults to False.
        evaluations (int, optional): number of times the benchmark is executed. Default is 10
        path_to_output_folder (Path, optional)
        remove_generated_file (bool, optional): remove the generated file at the end of the evaluation, Default is True

    Returns:
        tuple[int,list[float],list[float]]: file_size, list of throughputs, and list of memory usage
    """

    print(
        f"Evaluating parameters: {compression_type = }, {cluster_size = }, {page_size = }, {cluster_bunch = }")
    print(f"{benchmark = }")

    # Get Paths
    path_to_generator = path_to_iotools / f"gen_{benchmark}"
    path_to_benchmark = path_to_iotools / f"{benchmark}"
    path_to_reference_file = path_to_reference_files / \
        f"{benchmark_datafile_dict[benchmark]}.root"

    # Generate RNTuple and get the path to it
    path_to_generated_file = generate_file(path_to_generator, path_to_reference_file,
                                           compression_type, cluster_size, page_size, path_to_output_folder)

    generated_file_size = get_size(path_to_generated_file)

    # Run the benchmark on the generated file
    throughputs, memory_usages = run_benchmark(
        path_to_benchmark, path_to_generated_file, cluster_bunch, use_rdf, evaluations)

    if remove_generated_file:
        os.remove(path_to_generated_file)

    return generated_file_size, throughputs, memory_usages


def evaluate_default_parameters(benchmark: str, evaluations: int = 10) -> tuple[int, list[float], list[float]]:
    """ Get the performance of the default parameters usingf the basic evaluate_parameter function

    Args:
        benchmark (str):
        evaluations (int, optional): Defaults to 10.

    Returns:
        tuple[int,list[float],list[float]]: file_size, list of throughputs, and list of memory usage
    """

    return evaluate_parameters(benchmark, default_variable_values["compression_type"], default_variable_values["cluster_size"],
                               default_variable_values["page_size"], default_variable_values["cluster_bunch"],
                               default_variable_values["use_rdf"], evaluations)
