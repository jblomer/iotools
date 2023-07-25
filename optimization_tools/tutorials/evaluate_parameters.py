import add_path

import numpy as np
import argparse

from Benchmarking.benchmark_utils import evaluate_default_parameters, evaluate_parameters, \
    get_size_decrease, get_throughput_increase, get_memory_usage_decrease, get_performance
from Benchmarking.variables import benchmark_datafile_dict, compression_types, default_variable_values


# Add terminal arguments
parser = argparse.ArgumentParser()

parser.add_argument('-benchmark', choices=benchmark_datafile_dict, type=str,
                    default="lhcb")
parser.add_argument('-compression_type', choices=compression_types, type=str,
                    default=default_variable_values["compression_type"])
parser.add_argument('-cluster_size', type=int,
                    default=default_variable_values["cluster_size"])
parser.add_argument('-page_size', type=int,
                    default=default_variable_values["page_size"])
parser.add_argument('-cluster_bunch', type=int,
                    default=default_variable_values["cluster_bunch"])
parser.add_argument('-evaluations', type=int, default=10)

args = parser.parse_args()

benchmark = args.benchmark
compression_type = args.compression_type
cluster_size = args.cluster_size
page_size = args.page_size
cluster_bunch = args.cluster_bunch
evaluations = args.evaluations

# Get the default performance for the chosen benchmark
default_file_size, default_throughputs, default_memory_usages = evaluate_default_parameters(
    benchmark, evaluations=evaluations)

default_mean_throughput = np.mean(default_throughputs)
default_mean_memory_usage = np.mean(default_memory_usages)

# Get performance of the given parameters on the chosen benchmark
generated_file_size, throughputs, memory_usages = evaluate_parameters(benchmark, compression_type, cluster_size,
                                                                      page_size, cluster_bunch, evaluations=evaluations)

mean_throughput = np.mean(throughputs)
mean_memory_usage = np.mean(memory_usages)

# Get the relative performance of the parameters comapred to the default parameter set
size_decrease = get_size_decrease(generated_file_size, default_file_size)
throughput_increase = get_throughput_increase(
    mean_throughput, default_mean_throughput)
memory_usage_decrease = get_memory_usage_decrease(
    mean_memory_usage, default_mean_memory_usage)
performance = get_performance(
    [size_decrease, throughput_increase, memory_usage_decrease])

# Print performance metrics of the default parameter set
print(f"#" * 100)
print(f"The size of the default file is {default_file_size}")
print(
    f"The default parameters reached an average throughput of {default_mean_throughput:.2f}")
print(
    f"The default parameters had an average maximum memory usage of {default_mean_memory_usage:.2f}")

# Print performance metrics of the given parameter set
print(f"#" * 100)
print(f"The size of the generated file is {generated_file_size}")
print(
    f"The new parameters reached an average throughput of {mean_throughput:.2f}")
print(
    f"The new parameters had an average maximum memory usage of {mean_memory_usage:.2f}")

# Print the relative performance of the given parameters
print(f"#" * 100)
print(f"The filesize decreased by {size_decrease:.2f}%")
print(f"The throughput increasd by {throughput_increase:.2f}%")
print(f"The memory usage decreased by {memory_usage_decrease:.2f}%")

print(f"\nOverall performance is {performance:.2f}")
