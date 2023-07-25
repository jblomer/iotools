import add_path

from Benchmarking.Algorithms import Annealer
from Benchmarking.DataStructures.Configuration import getConfiguration

# Example of how to run the simmulated annealer on all four benchmarks
# For all benchmarks, lz4 and zstd are optimizated seperately.

evolution_steps = 200


def run_annealer(benchmark: str, compression_type: str, evolution_steps: int, multi_change: bool = False):
    # Create a configuration where the compression type is set to the given compression_type, and can never be mutated.
    conf = getConfiguration(compression_type=compression_type,
                            compression_types=[compression_type])
    a = Annealer(configuration=conf, benchmark=benchmark,
                 multi_change=multi_change)
    a.evolve(steps=evolution_steps)


# LHCb
run_annealer("lhcb", "lz4", evolution_steps, multi_change=False)
run_annealer("lhcb", "zstd", evolution_steps, multi_change=False)

# Atlas
run_annealer("atlas", "lz4", evolution_steps, multi_change=False)
run_annealer("atlas", "zstd", evolution_steps, multi_change=False)

# CMS
run_annealer("cms", "lz4", evolution_steps, multi_change=False)
run_annealer("cms", "zstd", evolution_steps, multi_change=False)

# H1
run_annealer("h1", "lz4", evolution_steps, multi_change=False)
run_annealer("h1", "zstd", evolution_steps, multi_change=False)
