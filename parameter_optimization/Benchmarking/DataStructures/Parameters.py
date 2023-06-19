from dataclasses import dataclass, field
from random import randint, choice, uniform


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


@dataclass
class Parameter:
    parameter_name: str

    def initialize(self):
        raise NotImplementedError

    def step(self):
        raise NotImplementedError

    def revert(self):
        raise NotImplementedError

    @property
    def value(self):
        raise NotImplementedError

    @property
    def can_mutate(self):
        raise NotImplementedError

    def __str__(self) -> str:
        raise NotImplementedError


@dataclass
class ListBasedParameter(Parameter):
    allowed_values: list
    value_names: list[str] = None
    current_idx: int = None
    previous_idx: int = None

    def __post_init__(self):
        if self.value_names == None:
            self.value_names = [str(x) for x in self.allowed_values]

        if self.current_idx == None:
            self.randomize()

    @property
    def can_mutate(self) -> bool:
        return len(self.allowed_values) > 1

    def randomize(self):
        """ Set the value to a random value
        """
        self.current_idx = randint(0, len(self.allowed_values) - 1)

    def revert(self):
        """ Revert Parameter to the previous state

        Raises:
            ValueError: No previous state is available
        """
        if self.previous_idx == None:
            raise ValueError("No previous idx available")
        self.current_idx = self.previous_idx

    @property
    def value(self) -> list:
        return self.allowed_values[self.current_idx]

    def __str__(self) -> str:
        if self.current_idx == None:
            return f"Valiable {self.parameter_name} is not yet initialized"

        return f"{self.parameter_name} \t=> {self.value_names[self.current_idx]}"


@dataclass
class DiscreteParameter(ListBasedParameter):
    def step(self):
        """ Make a step by either increasing, or decreasing the current index
        """

        self.previous_idx = self.current_idx
        if self.current_idx == 0:
            self.current_idx = 1
            return

        if self.current_idx == len(self.allowed_values) - 1:
            self.current_idx = len(self.allowed_values) - 2
            return

        self.current_idx = choice([self.current_idx - 1,
                                   self.current_idx + 1])


@dataclass
class CategoricalParameter(ListBasedParameter):
    def step(self):
        """ Make a step by chosing a random new index
        """
        self.previous_idx = self.current_idx
        self.current_idx = choice(
            [x for x in range(len(self.allowed_values)) if x != self.current_idx])


@dataclass
class ContinuousParameter(Parameter):
    lower_bound: float
    upper_bound: float
    value: float = None
    previous_value: float = None

    @property
    def value(self) -> float:
        return self._value

    @value.setter
    def value(self, value: float):
        # Handle default value
        if isinstance(value, property):
            self.randomize_value()
            return

        # Bound the value between upper and lower bound
        value = self.upper_bound if value > self.upper_bound else value
        value = self.lower_bound if value < self.lower_bound else value

        self._value = value

    def randomize_value(self):
        """ Set the value to a random value within the bounds
        """
        self.value = uniform(self.lower_bound, self.upper_bound)

    @property
    def max_step_size(self) -> float:
        return (self.upper_bound - self.lower_bound) / 10

    @property
    def can_mutate(self) -> bool:
        return True

    def step(self):
        """ Make a step by taking a step of a random size between -max_step and max_step. 
            max_step is 10% of the allowed range of the Parameter.
            With be capped to the bounds if it exceeds it
        """
        self.previous_value = self.value

        step_size = self.max_step_size
        direction = uniform(-step_size, step_size)

        self.value += direction

    def revert(self):
        """ Revert Parameter to the previous state

        Raises:
            ValueError: No previous state is available
        """
        if self.previous_value is None:
            raise ValueError("No previous value available")

        self.value = self.previous_value


###################################################################################################
# Utils
###################################################################################################

def getCompressionParameter(current_value: str, compression_types: list[str] = ["none", "zlib", "lz4", "lzma", "zstd"]) -> CategoricalParameter:

    if current_value not in compression_types:
        raise ValueError(
            f"{current_value = } is not a valid value out {compression_types = }")
    current_idx = compression_types.index(current_value)

    return CategoricalParameter("Compression Type", compression_types, current_idx=current_idx)


def getPageSizeParameter(current_value) -> DiscreteParameter:
    page_sizes = [(16, "KB"), (32, "KB"), (64, "KB"), (128, "KB"), (256, "KB"), (512, "KB"),
                  (1, "MB"), (2, "MB"), (4, "MB"), (8, "MB"), (16, "MB")]

    value_names = convertByteToStr(page_sizes)
    page_sizes = convertToByteList(page_sizes)

    if current_value not in page_sizes:
        raise ValueError(
            f"{current_value = } is not a valid value out {page_sizes = }")
    current_idx = page_sizes.index(current_value)

    return DiscreteParameter("Page Size", page_sizes, value_names=value_names, current_idx=current_idx)


def getClusterSizeParameter(current_value) -> DiscreteParameter:
    cluster_sizes = [(20, "MB"), (30, "MB"), (40, "MB"), (50, "MB"), (100, "MB"), (200, "MB"),
                     (300, "MB"), (400, "MB"), (500, "MB")]

    value_names = convertByteToStr(cluster_sizes)

    cluster_sizes = convertToByteList(cluster_sizes)

    if current_value not in cluster_sizes:
        raise ValueError(
            f"{current_value = } is not a valid value out {cluster_sizes = }")
    current_idx = cluster_sizes.index(current_value)

    return DiscreteParameter("Cluster Size", cluster_sizes, value_names=value_names, current_idx=current_idx)


def getClusterBunchParameter(current_value: int) -> DiscreteParameter:
    cluster_bunches = [1, 2, 3, 4, 5]
    if current_value not in cluster_bunches:
        raise ValueError(
            f"{current_value = } is not a valid value out {cluster_bunches = }")
    current_idx = cluster_bunches.index(current_value)

    return DiscreteParameter("Cluster Bunch", cluster_bunches, current_idx=current_idx)
