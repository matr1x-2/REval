from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class CReval:
    """C evaluation constants and schema hints."""

    MAX_INPUTS = 5


REQUIRED_DATA_KEYS = [
    "task_id",
    "entry_point",
    "code",
    "inputs",
]


def validate_record_shape(record: dict[str, Any]) -> None:
    for key in REQUIRED_DATA_KEYS:
        if key not in record:
            raise ValueError(f"Missing required key: {key}")
    if not isinstance(record["inputs"], list) or len(record["inputs"]) == 0:
        raise ValueError("`inputs` must be a non-empty list")
    for idx, inp in enumerate(record["inputs"]):
        if not isinstance(inp, dict):
            raise ValueError(f"inputs[{idx}] must be an object")
        if "harness" not in inp:
            raise ValueError(f"inputs[{idx}] missing required key: harness")
        if "stubs" in inp and not isinstance(inp["stubs"], str):
            raise ValueError(f"inputs[{idx}].stubs must be a string")
