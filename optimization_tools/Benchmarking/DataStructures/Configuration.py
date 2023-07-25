from dataclasses import dataclass
from random import choice
import os
import numpy as np
from pathlib import Path


from Benchmarking.benchmark_utils import convertToByteList, convertByteToStr, evaluate_parameters
from Benchmarking.DataStructures.Parameters import Parameter, CategoricalParameter, DiscreteParameter, \
    getCompressionParameter, getClusterSizeParameter, getClusterBunchParameter, getPageSizeParameter

from Benchmarking.variables import path_to_generated_files, default_variable_values


###############################################################################################################################
# Configuration
###############################################################################################################################

@dataclass
class Configuration:
    compression_type: CategoricalParameter = None
    cluster_size: DiscreteParameter = None
    page_size: DiscreteParameter = None
    cluster_bunch: DiscreteParameter = None

    parameters: list[Parameter] = None
    mutatable_parameters: list[Parameter] = None
    mutated_variable: Parameter = None

    mutated_parameters: list[Parameter] = None

    def __post_init__(self):
        if self.compression_type == None:
            self.createBaseConfig()

        self.parameters = [self.compression_type,
                           self.cluster_size,
                           self.page_size,
                           self.cluster_bunch]

        self.mutatable_parameters = [
            parameter for parameter in self.parameters if parameter.can_mutate]

    @property
    def names(self) -> list[str]:
        return [parameter.parameter_name for parameter in self.parameters]

    @property
    def values(self) -> list[str]:
        return [parameter.value for parameter in self.parameters]

    def createBaseConfig(self):
        """ Creates the configuration that is the default configuration defined by ROOT
        """
        self.compression_type = CategoricalParameter(
            "Compression Type", ["none", "zlib", "lz4", "lzma", "zstd"], current_idx=2)

        page_sizes = [(16, "KB"), (32, "KB"), (64, "KB"), (128, "KB"), (256, "KB"), (512, "KB"),
                      (1, "MB"), (2, "MB"), (4, "MB"), (8, "MB"), (16, "MB")]
        self.page_size = DiscreteParameter("Page Size", convertToByteList(page_sizes),
                                           value_names=convertByteToStr(page_sizes), current_idx=2)

        cluster_sizes = [(20, "MB"), (30, "MB"), (40, "MB"), (50, "MB"), (100, "MB"), (200, "MB"),
                         (300, "MB"), (400, "MB"), (500, "MB")]
        self.cluster_size = DiscreteParameter("Cluster Size", convertToByteList(cluster_sizes),
                                              value_names=convertByteToStr(cluster_sizes), current_idx=3)

        self.cluster_bunch = DiscreteParameter(
            "Cluster Bunch", [1, 2, 3, 4, 5], current_idx=0)

    def randomize(self):
        """ Change all parameters to a random value
        """
        for var in self.parameters:
            var.initialize()

    def step(self):
        """ Make a random variable set a step
        """
        self.mutated_variable = self.mutatable_parameters[choice(
            range(len(self.mutatable_parameters)))]
        self.mutated_variable.step()

    def step_multi(self, prop):
        x = np.arange(len(self.mutatable_parameters)) + 1
        number_of_changes = np.random.choice(x, p=prop)

        idxs_to_change = np.random.choice(
            x - 1, number_of_changes, replace=False)

        self.mutated_mutatable_parameters = []
        for i in idxs_to_change:
            self.mutatable_parameters[i].step()
            self.mutated_mutatable_parameters.append(
                self.mutatable_parameters[i])

        return number_of_changes

    def revert(self):
        """ Revert the previous step
        """
        self.mutated_variable.revert()

    def revert_multi(self):
        for var in self.mutated_parameters:
            var.revert()

        self.mutated_parameters = []

    def __str__(self) -> str:
        s = f"Current configuration:\n"

        for var in self.parameters:
            s += f"{var.__str__()}\n"

        return s

    def evaluate(self, benchmark: str, evaluations: int = 10, path_to_output_folder: Path = path_to_generated_files,
                 remove_generated_file: bool = True) -> list[float]:
        """ Evaluate the current configuration using a specific benchmark. 
            The benchmark is generated, and run multiple times.

        Args:
            benchmark_file (str): The type of benchmark to generate
            data_file (str): The data file that is used for generation
            evaluations (int, optional): number of times the benchmark should be run. Defaults to 10.

        Returns:
            list[float]: The throughput of each run
        """

        return evaluate_parameters(benchmark, self.compression_type.value, self.cluster_size.value, self.page_size.value,
                                   self.cluster_bunch.value, path_to_output_folder=path_to_output_folder,
                                   evaluations=evaluations, remove_generated_file=remove_generated_file)


def getConfiguration(compression_type: str = default_variable_values["compression_type"], cluster_size: int = default_variable_values["cluster_size"],
                     page_size: int = default_variable_values["page_size"], cluster_bunch: int = default_variable_values["cluster_bunch"],
                     compression_types: list[str] = None):

    if compression_types:
        compression_type = getCompressionParameter(
            compression_type, compression_types=compression_types)
    else:
        compression_type = getCompressionParameter(compression_type)

    page_size = getPageSizeParameter(int(page_size))
    cluster_size = getClusterSizeParameter(int(cluster_size))
    cluster_bunch = getClusterBunchParameter(int(cluster_bunch))

    return Configuration(compression_type=compression_type,
                         cluster_size=cluster_size,
                         page_size=page_size,
                         cluster_bunch=cluster_bunch)
