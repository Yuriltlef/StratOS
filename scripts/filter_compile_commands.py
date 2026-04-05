#!/usr/bin/env python3
"""
Filter out assembler file entries from CMake's compile_commands.json.

Usage:
    filter_compile_commands.py [--output-dir OUTPUT_DIR]

If --output-dir is provided, the filtered compile_commands.json is written to that
directory (the file name is always compile_commands.json). Otherwise, the original
file is overwritten in place.

The script automatically locates the project root by looking for a 'scripts'
directory, and expects the original compile_commands.json to be in the 'build'
subdirectory of the project root.
"""

import json
import os
import sys
import argparse
from pathlib import Path
from typing import List, Dict, Any


class CompilationDatabaseFilter:
    """Filter out assembler entries from a CMake compile_commands.json file."""

    ASSEMBLER_EXTENSIONS = {'.s', '.S', '.asm'}

    def __init__(self, start_path: Path = None) -> None:
        if start_path is None:
            start_path = Path(__file__).parent.resolve()
        self.start_path = start_path
        self.project_root = self._find_project_root()
        self.build_dir = self.project_root / "build"
        self.source_db_path = self.build_dir / "compile_commands.json"

    def _find_project_root(self) -> Path:
        current = self.start_path
        while current != current.parent:
            if current.name == "scripts" and (current.parent / "build").exists():
                return current.parent
            current = current.parent
        raise RuntimeError(
            "Could not locate project root. "
            "Make sure the script resides in a 'scripts' directory and "
            "that a 'build' directory exists at the same level as 'scripts'."
        )

    def _is_assembler_file(self, file_path: str) -> bool:
        path_lower = file_path.lower()
        for ext in self.ASSEMBLER_EXTENSIONS:
            if path_lower.endswith(ext):
                return True
        return False

    def load_database(self) -> List[Dict[str, Any]]:
        if not self.source_db_path.is_file():
            raise FileNotFoundError(f"compile_commands.json not found at {self.source_db_path}")
        with open(self.source_db_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        if not isinstance(data, list):
            raise ValueError("Expected compile_commands.json to contain a JSON array.")
        return data

    def filter_assembler_entries(self, entries: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        filtered = []
        for entry in entries:
            file_field = entry.get("file", "")
            if not file_field:
                filtered.append(entry)
                continue
            if not self._is_assembler_file(file_field):
                filtered.append(entry)
        return filtered

    def save_database(self, entries: List[Dict[str, Any]], output_path: Path) -> None:
        """Save filtered database to the given output path (overwrites)."""
        output_path.parent.mkdir(parents=True, exist_ok=True)
        temp_path = output_path.with_suffix(".tmp")
        with open(temp_path, 'w', encoding='utf-8') as f:
            json.dump(entries, f, indent=2)
        os.replace(temp_path, output_path)
        print(f"Filtered compile_commands.json written to {output_path}")

    def run(self, output_dir: Path = None) -> None:
        """
        Execute filtering. If output_dir is provided, write the filtered database
        to output_dir/compile_commands.json; otherwise overwrite the original.
        """
        print(f"Project root: {self.project_root}")
        print(f"Original database: {self.source_db_path}")

        original = self.load_database()
        original_count = len(original)
        print(f"Loaded {original_count} entries.")

        filtered = self.filter_assembler_entries(original)
        filtered_count = len(filtered)
        removed_count = original_count - filtered_count
        print(f"Removed {removed_count} assembler file entries.")
        print(f"Keeping {filtered_count} entries.")

        if removed_count == 0:
            print("No assembler entries found. Nothing to do.")
            return

        if output_dir is not None:
            output_path = Path(output_dir) / "compile_commands.json"
        else:
            output_path = self.source_db_path

        self.save_database(filtered, output_path)
        print("Filtering completed successfully.")


def main():
    parser = argparse.ArgumentParser(
        description="Filter assembler entries from compile_commands.json"
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        help="Directory where the filtered compile_commands.json will be written. "
             "If not specified, the original file is overwritten."
    )
    args = parser.parse_args()

    output_dir = Path(args.output_dir) if args.output_dir else None

    filter_tool = CompilationDatabaseFilter()
    filter_tool.run(output_dir=output_dir)


if __name__ == "__main__":
    main()