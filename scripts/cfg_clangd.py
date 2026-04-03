#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
generate_clangd.py

This script generates a .clangd configuration file for the StratOS project
based on a user-supplied JSON configuration and a YAML template.

It replaces placeholders in the template:
- {your_compiler_path}      -> path to arm-none-eabi-g++
- {your_toolchain_path}     -> toolchain root directory
- {project_root}            -> absolute project root path (converted to forward slashes)
- {project_definitions}     -> list of -D macros from the JSON
- {g++_predefines}          -> all predefined macros from the actual compiler

The output is written to .clangd.test in the project root directory.

Usage:
    python scripts/generate_clangd.py
"""

import json
import subprocess
import sys
import os
import re
from pathlib import Path
from typing import Dict, List, Optional


class ClangdConfigGenerator:
    """Generates a .clangd configuration file from a template and a JSON config."""

    def __init__(self, config_path: Path, template_path: Path, output_path: Path) -> None:
        """
        Initialize the generator.

        Args:
            config_path: Path to the JSON configuration file.
            template_path: Path to the YAML template file.
            output_path: Path where the generated .clangd file will be written.
        """
        self.config_path = config_path
        self.template_path = template_path
        self.output_path = output_path
        self.config: Dict = {}
        self.template_lines: List[str] = []

    def run(self) -> None:
        """Execute the generation process."""
        try:
            self.load_config()
            self.load_template()
            processed_lines = self.process_template()
            self.write_output(processed_lines)
            print(f"Successfully generated {self.output_path}")
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)

    def load_config(self) -> None:
        """Load and validate the JSON configuration file."""
        if not self.config_path.is_file():
            raise FileNotFoundError(f"Configuration file not found: {self.config_path}")

        try:
            with open(self.config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON in {self.config_path}: {e}")

        # Validate required fields
        clangd = self.config.get("clangd")
        if not clangd:
            raise KeyError("Missing top-level 'clangd' key in configuration")

        required = ["g++_compiler_path", "toolchain_root_path", "project_root", "project_definitions"]
        for field in required:
            if field not in clangd:
                raise KeyError(f"Missing required field 'clangd.{field}' in configuration")

        # Ensure project_definitions is a list
        if not isinstance(clangd["project_definitions"], list):
            raise TypeError("'clangd.project_definitions' must be a list of strings")

    def load_template(self) -> None:
        """Read the YAML template file into a list of lines."""
        if not self.template_path.is_file():
            raise FileNotFoundError(f"Template file not found: {self.template_path}")

        with open(self.template_path, 'r', encoding='utf-8') as f:
            self.template_lines = f.readlines()

    def get_gcc_predefines(self) -> List[str]:
        """
        Execute the compiler to obtain all predefined macros.

        Returns:
            A list of strings, each representing a compiler flag: '-DNAME=VALUE' or '-DNAME'.
        """
        compiler = self.config["clangd"]["g++_compiler_path"]
        if not os.path.isfile(compiler):
            raise FileNotFoundError(f"Compiler not found: {compiler}")

        # Build command: use -E -dM -x c++ -std=c++17 and read from null device
        cmd = [compiler, "-E", "-dM", "-x", "c++", "-std=c++17", "-"]
        try:
            # Use subprocess with no input (reading from /dev/null or nul)
            result = subprocess.run(
                cmd,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True
            )
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Failed to run compiler {compiler}: {e.stderr}") from e

        flags = []
        for line in result.stdout.splitlines():
            line = line.strip()
            if line.startswith("#define "):
                parts = line[8:].split(maxsplit=1)
                macro = parts[0]
                value = parts[1] if len(parts) > 1 else ""
                if value:
                    flags.append(f"-D{macro}={value}")
                else:
                    flags.append(f"-D{macro}")
        return flags

    def process_template(self) -> List[str]:
        """
        Replace placeholders in the template line by line.

        Returns:
            A list of lines with all placeholders replaced.
        """
        processed_lines = []
        # Cache for multiline replacements
        proj_defs_lines: Optional[List[str]] = None
        gcc_predef_lines: Optional[List[str]] = None

        # Precompute simple replacements
        simple_replacements = {
            "{your_compiler_path}": self.normalize_path(self.config["clangd"]["g++_compiler_path"]),
            "{your_toolchain_path}": self.normalize_path(self.config["clangd"]["toolchain_root_path"]),
            # Convert backslashes to forward slashes for YAML compatibility
            "{project_root}": self.normalize_path(self.config["clangd"]["project_root"].replace("\\", "/")),
        }

        for line in self.template_lines:
            # Check for multiline placeholders
            if "{project_definitions}" in line:
                if proj_defs_lines is None:
                    proj_defs_lines = self._generate_project_definitions(line)
                processed_lines.extend(proj_defs_lines)
                continue

            if "{g++_predefines}" in line:
                if gcc_predef_lines is None:
                    gcc_predef_lines = self._generate_gcc_predefines(line)
                processed_lines.extend(gcc_predef_lines)
                continue

            # Simple placeholder replacement
            for placeholder, value in simple_replacements.items():
                if placeholder in line:
                    line = line.replace(placeholder, value)
            processed_lines.append(line)

        return processed_lines

    def _generate_project_definitions(self, template_line: str) -> List[str]:
        """
        Generate lines for project-specific preprocessor definitions.

        Args:
            template_line: The line from the template that contains the placeholder.

        Returns:
            A list of lines, each with proper indentation and a '-D' flag.
        """
        indent = self._get_indent(template_line)
        definitions = self.config["clangd"]["project_definitions"]
        lines = []
        for macro in definitions:
            lines.append(f"{indent}- -D{macro}\n")
        # If no definitions, output a comment line (optional)
        if not lines:
            lines.append(f"{indent}# No project definitions provided\n")
        return lines

    def _generate_gcc_predefines(self, template_line: str) -> List[str]:
        """
        Generate lines for compiler predefined macros.

        Args:
            template_line: The line from the template that contains the placeholder.

        Returns:
            A list of lines, each with proper indentation and a '-D' flag.
        """
        indent = self._get_indent(template_line)
        try:
            predefines = self.get_gcc_predefines()
        except Exception as e:
            # Fallback: add a comment line indicating failure
            print(f"Warning: Could not obtain predefined macros: {e}", file=sys.stderr)
            return [f"{indent}# Failed to obtain compiler predefines: {e}\n"]

        lines = []
        for flag in predefines:
            lines.append(f"{indent}- {flag}\n")
        # Limit output size to avoid huge files? Usually acceptable.
        return lines

    @staticmethod
    def _get_indent(line: str) -> str:
        """Extract the leading whitespace from a line."""
        match = re.match(r"^\s*", line)
        return match.group(0) if match else ""

    def write_output(self, lines: List[str]) -> None:
        """Write the processed lines to the output file."""
        # Ensure output directory exists (in case output path is nested)
        self.output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(self.output_path, 'w', encoding='utf-8') as f:
            f.writelines(lines)
    
    @staticmethod
    def normalize_path(path: str) -> str:
        """Convert to forward slashes and remove trailing slash (except root)."""
        path = path.replace("\\", "/")
        if path.endswith("/") and not path == "/":
            path = path.rstrip("/")
        return path


def main():
    # Determine project root: script is in root/scripts/, so root is parent of parent
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    # Default file locations
    config_file = project_root / "clangd_config.json"
    template_file = project_root / ".clangd.template.yml"
    output_file = project_root / ".clangd"

    # Allow overriding via environment variables (optional)
    if "CLANGD_CONFIG" in os.environ:
        config_file = Path(os.environ["CLANGD_CONFIG"])
    if "CLANGD_TEMPLATE" in os.environ:
        template_file = Path(os.environ["CLANGD_TEMPLATE"])
    if "CLANGD_OUTPUT" in os.environ:
        output_file = Path(os.environ["CLANGD_OUTPUT"])

    generator = ClangdConfigGenerator(config_file, template_file, output_file)
    generator.run()


if __name__ == "__main__":
    main()