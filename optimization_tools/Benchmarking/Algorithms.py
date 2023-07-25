# %%

# root [12] auto ntuple = ROOT::Experimental::RNTupleReader::Open("DecayTree" , "generated/B2HHH~none.ntuple")
# root [13] ntuple->PrintInfo(ROOT::Experimental::ENTupleInfo::kStorageDetails)

from dataclasses import dataclass
from datetime import datetime
import numpy as np
from pathlib import Path
from random import random

from Benchmarking.DataStructures.Configuration import Configuration
from Benchmarking.benchmark_utils import get_size_decrease, get_throughput_increase, get_memory_usage_decrease, get_performance, evaluate_default_parameters
from Benchmarking.variables import default_variable_values, path_to_results

# %%


@dataclass
class Walker:
    configuration: Configuration = None

    # benchmark definitions
    benchmark: str = "lhcb"

    # Performance parameters
    default_file_size: int = None
    default_throughput: float = None
    default_memory_usage: float = None
    performance: float = None

    # TODO: add weights
    weights: list[float] = None

    write_file: Path = path_to_results

    multi_change: bool = False

    def __post_init__(self):
        if self.configuration == None:
            self.configuration = Configuration()

        class_name = self.get_class_name()

        folder_path = self.write_file / f'{self.benchmark}/{class_name}'
        folder_path.mkdir(parents=True, exist_ok=True)

        self.write_file = folder_path / \
            f'{datetime.now().strftime("%y-%m-%d_%H:%M:%S")}.csv'

    def get_class_name(self) -> str:
        raise NotImplementedError

    def is_accepted(self, performance: float, iteration: int) -> bool:
        raise NotImplementedError

    change_probabilities: list[list[float]] = None

    def create_probability_list(self, iterations: int):
        x = np.arange(len(self.configuration.mutatable_parameters)) + 1
        prop = x[::-1] / np.sum(x)
        last_prop = np.zeros(len(self.configuration.mutatable_parameters))
        last_prop[0] = 1

        self.change_probabilities = np.linspace(prop, last_prop, iterations)

    def get_num_changes(self, iteration: int) -> list[float]:
        if iteration > len(self.change_probabilities):
            return self.change_probabilities[-1]

        return self.change_probabilities[iteration]

    def aggregate_performance(self, throughputs, memory_usages):

        mean_throughput = np.mean(throughputs)
        mean_memory_usage = np.mean(memory_usages)
        return mean_throughput, mean_memory_usage

    def normalize_performance(self, file_size: int, mean_throughput: float, mean_memory_usage: float) -> tuple[float, float, float, float]:

        size_decrease = get_size_decrease(
            file_size, self.default_file_size)
        throughput_increase = get_throughput_increase(
            mean_throughput, self.default_throughput)
        memory_usage_decrease = get_memory_usage_decrease(
            mean_memory_usage, self.default_memory_usage)

        performance = get_performance(
            [size_decrease, throughput_increase, memory_usage_decrease], self.weights)

        return size_decrease, throughput_increase, memory_usage_decrease, performance

    def log_step(self, generated_file_size: int, throughputs: list[float], memory_usages: list[float], mean_throughput: float,
                 mean_memory_usage: float, size_decrease: float, throughput_increase: float, memory_usage_decrease: float,
                 performance: float, accepted: bool = True, is_default: bool = False):
        """ Log a taken step

        Args:
            results (list[float])
            throughput (float)
            throughput_increase (float)
            size (int)
            size_decrease (float)
            performance (float, optional): Defaults to 1.0.
            accepted (bool, optional): Defaults to False.
        """
        with open(self.write_file, "a") as wf:
            wf.write(f'{datetime.now().strftime("%y-%m-%d_%H:%M:%S")},')

            if is_default:
                wf.write(f"{default_variable_values['compression_type']},")
                wf.write(f"{default_variable_values['cluster_size']},")
                wf.write(f"{default_variable_values['page_size']},")
                wf.write(f"{default_variable_values['cluster_bunch']},")
            else:
                for value in self.configuration.values:
                    wf.write(f"{value},")

            wf.write(f"{accepted},{performance:.2f},{size_decrease:.2f},{throughput_increase:.3f},{memory_usage_decrease:.3f},{generated_file_size},{mean_throughput:.3f},{mean_memory_usage:.1f}")

            for throughput in throughputs:
                wf.write(f",{throughput:.1f}")
            for memory_usage in memory_usages:
                wf.write(f",{memory_usage:.1f}")

            wf.write("\n")

    def step(self, iteration: int, evaluations: int = 10):
        """ Change the current configuration. 
            Determine the performance of the new configuration.
            Determine if you want to keep the new configuration.

        Args:
            iteration (int)
            evaluations (int, optional). Defaults to 10.
        """
        if iteration > 0:
            if self.multi_change:
                self.configuration.step_multi(self.get_num_changes(iteration))
            else:
                self.configuration.step()

        generated_file_size, throughputs, memory_usages = self.configuration.evaluate(
            self.benchmark, evaluations=evaluations)

        mean_throughput, mean_memory_usage = self.aggregate_performance(
            throughputs, memory_usages)

        size_decrease, throughput_increase, memory_usage_decrease, performance = self.normalize_performance(
            generated_file_size, mean_throughput, mean_memory_usage)

        accepted = self.is_accepted(performance, iteration)

        # Logging
        if accepted:
            self.log_step(generated_file_size, throughputs, memory_usages, mean_throughput, mean_memory_usage,
                          size_decrease, throughput_increase, memory_usage_decrease, performance, accepted=True)
            self.performance = performance

        else:
            self.log_step(generated_file_size, throughputs, memory_usages, mean_throughput, mean_memory_usage,
                          size_decrease, throughput_increase, memory_usage_decrease, performance, accepted=False)
            self.configuration.revert()

    def get_default_performance(self, evaluations: int = 10):
        """ Get the performance of the "base" configuration. 
            This performance will be used to normalize all other results

        Args:
            evaluations (int, optional) Defaults to 10.
        """
        generated_file_size, throughputs, memory_usages = evaluate_default_parameters(
            self.benchmark, evaluations)

        mean_throughput, mean_memory_usage = self.aggregate_performance(
            throughputs, memory_usages)

        self.performance = 0
        self.default_file_size = generated_file_size
        self.default_throughput = mean_throughput
        self.default_memory_usage = mean_memory_usage

        self.log_step(generated_file_size, throughputs, memory_usages, mean_throughput, mean_memory_usage,
                      0, 0, 0, -999, accepted=False, is_default=True)

    def evolve(self, steps: int = 100, evaluations: int = 10):
        """ Evolve the current configuration using the simmulated annealing algorithm

        Args:
            steps (int, optional): Number of steps to evolve for. Defaults to 100.
            evaluations (int, optional): Number of times to run a benchmark with a configuration. Defaults to 10.
        """

        with open(self.write_file, "w") as wf:
            wf.write("time,")

            for name in self.configuration.names:
                wf.write(f"{name},")

            wf.write(f"accepted,performance(%),size_decrease(%),throughput_increase(%),memory_usage_decrease(%),size(MB),mean_throughput(MB/s),mean_memory_usage")

            for i in range(evaluations):
                wf.write(f",throughput_{i}")
            for i in range(evaluations):
                wf.write(f",memory_usage_{i}")

            wf.write("\n")

        # Log initial configuration
        print(f"Calculating Initial Performance")
        self.get_default_performance(evaluations)

        # # evolve the configuration for the given number of steps
        print(f"Starting evolution")
        for i in range(steps):
            print(f"Step: {i} => Throughput: {self.performance:.3f}")
            self.step(i, evaluations)


######################################################################################################
# Random Walker
######################################################################################################

class RandomWalker(Walker):
    def is_accepted(self, performance: float, iteration: int) -> bool:
        return True

    def get_class_name(self) -> str:
        return "RandomWalker"

######################################################################################################
# HillClimber
######################################################################################################


class HillClimber(Walker):
    def is_accepted(self, performance: float, iteration: int) -> bool:
        return performance > self.performance

    def get_class_name(self) -> str:
        return "RandomWalker"

######################################################################################################
# Simmulated annealer
######################################################################################################


@dataclass
class Annealer(Walker):
    # Annealer parameters
    temperature_const: float = 2.5
    iteration: int = 0

    def get_class_name(self) -> str:
        return "Annealer"

    def get_temperature(self, iteration: int) -> float:
        """ Get temperature based on the current iteration

        Args:
            iteration (int)

        Returns:
            float
        """
        return self.temperature_const / np.log(iteration + 2)

    def get_probability(self, iteration: int, c: float) -> float:
        """ Get the probability of accepting a change based on 
        the current iteration and the difference in performance

        Args:
            iteration (int)
            c (float): The difference between the performance of 
                       the new and old configuration

        Returns:
            float
        """
        return np.exp(c / self.get_temperature(iteration))

    def is_accepted(self, performance: float, iteration: int) -> bool:
        # Determine if new configuration will be accepted
        c = performance - self.performance
        return (c > 0) or (self.get_probability(iteration, c) > random())
