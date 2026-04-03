#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
cfg_clangd.py

Generates .clangd configuration and clangd_fallback_flags.json for StratOS project.

Usage:
    python cfg_clangd.py
"""

import json
import subprocess
import sys
import os
import re
from pathlib import Path
from typing import Dict, List, Optional


class ClangdConfigGenerator:
    """Generates .clangd and fallback flags from a template and JSON config."""

    def __init__(self, config_path: Path, template_path: Path, output_path: Path) -> None:
        self.config_path = config_path
        self.template_path = template_path
        self.output_path = output_path
        self.config: Dict = {}
        self.template_lines: List[str] = []

    # -------------------------------------------------------------------------
    # Public entry point
    # -------------------------------------------------------------------------
    def run(self) -> None:
        self.load_config()
        self.load_template()
        processed_lines = self.process_template()
        self.write_output(processed_lines)
        self.generate_fallback_flags()
        print(f"Successfully generated {self.output_path}")
        print(f"Successfully generated fallback flags in project root")

    # -------------------------------------------------------------------------
    # Configuration loading
    # -------------------------------------------------------------------------
    def load_config(self) -> None:
        if not self.config_path.is_file():
            raise FileNotFoundError(f"Configuration file not found: {self.config_path}")

        try:
            with open(self.config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON in {self.config_path}: {e}")

        clangd = self.config.get("clangd")
        if not clangd:
            raise KeyError("Missing top-level 'clangd' key in configuration")

        required = ["g++_compiler_path", "toolchain_root_path", "project_root", "project_definitions"]
        for field in required:
            if field not in clangd:
                raise KeyError(f"Missing required field 'clangd.{field}' in configuration")

        if not isinstance(clangd["project_definitions"], list):
            raise TypeError("'clangd.project_definitions' must be a list of strings")

    def load_template(self) -> None:
        if not self.template_path.is_file():
            raise FileNotFoundError(f"Template file not found: {self.template_path}")

        with open(self.template_path, 'r', encoding='utf-8') as f:
            self.template_lines = f.readlines()

    # -------------------------------------------------------------------------
    # Path normalization
    # -------------------------------------------------------------------------
    @staticmethod
    def normalize_path(path: str) -> str:
        """Convert backslashes to forward slashes and remove trailing slash (except root)."""
        path = path.replace("\\", "/")
        if path.endswith("/") and not path == "/":
            path = path.rstrip("/")
        return path

    # -------------------------------------------------------------------------
    # Compiler interrogation
    # -------------------------------------------------------------------------
    def get_gcc_predefines(self) -> List[str]:
        """Return list of -D flags from compiler's predefined macros."""
        compiler = self.config["clangd"]["g++_compiler_path"]
        if not os.path.isfile(compiler):
            raise FileNotFoundError(f"Compiler not found: {compiler}")

        cmd = [compiler, "-E", "-dM", "-x", "c++", "-std=c++17", "-"]
        try:
            result = subprocess.run(
                cmd,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True,
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

    def get_system_include_paths(self) -> List[str]:
        """Return list of system include directories (for -isystem)."""
        compiler = self.config["clangd"]["g++_compiler_path"]
        if not os.path.isfile(compiler):
            return []

        cmd = [compiler, "-E", "-x", "c++", "-std=c++17", "-", "-v"]
        try:
            result = subprocess.run(
                cmd,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
        except Exception as e:
            print(f"Warning: Failed to get system includes: {e}", file=sys.stderr)
            return []

        lines = result.stderr.splitlines()
        paths = []
        start_marker = "#include <...> search starts here:"
        end_marker = "End of search list."
        in_section = False
        for line in lines:
            if start_marker in line:
                in_section = True
                continue
            if end_marker in line:
                break
            if in_section:
                # line may start with spaces; strip them
                path = line.strip()
                if path and not path.startswith("(framework directory)"):
                    paths.append(path)
        return paths

    # -------------------------------------------------------------------------
    # Template processing
    # -------------------------------------------------------------------------
    def process_template(self) -> List[str]:
        processed_lines = []
        proj_defs_lines: Optional[List[str]] = None
        gcc_predef_lines: Optional[List[str]] = None

        simple_replacements = {
            "{your_compiler_path}": self.normalize_path(self.config["clangd"]["g++_compiler_path"]),
            "{your_toolchain_path}": self.normalize_path(self.config["clangd"]["toolchain_root_path"]),
            "{project_root}": self.normalize_path(self.config["clangd"]["project_root"]),
        }

        for line in self.template_lines:
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

            for placeholder, value in simple_replacements.items():
                if placeholder in line:
                    line = line.replace(placeholder, value)
            processed_lines.append(line)

        return processed_lines

    def _generate_project_definitions(self, template_line: str) -> List[str]:
        indent = self._get_indent(template_line)
        definitions = self.config["clangd"]["project_definitions"]
        lines = []
        for macro in definitions:
            lines.append(f"{indent}- -D{macro}\n")
        if not lines:
            lines.append(f"{indent}# No project definitions provided\n")
        return lines

    def _generate_gcc_predefines(self, template_line: str) -> List[str]:
        indent = self._get_indent(template_line)
        try:
            predefines = self.get_gcc_predefines()
        except Exception as e:
            print(f"Warning: Could not obtain predefined macros: {e}", file=sys.stderr)
            return [f"{indent}# Failed to obtain compiler predefines: {e}\n"]

        lines = []
        for flag in predefines:
            lines.append(f"{indent}- {flag}\n")
        return lines

    @staticmethod
    def _get_indent(line: str) -> str:
        match = re.match(r"^\s*", line)
        return match.group(0) if match else ""

    def write_output(self, lines: List[str]) -> None:
        self.output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(self.output_path, 'w', encoding='utf-8') as f:
            f.writelines(lines)

    # -------------------------------------------------------------------------
    # Fallback flags generation
    # -------------------------------------------------------------------------
    def generate_fallback_flags(self) -> None:
        """Generate clangd_fallback_flags.json for VSCode clangd.fallbackFlags."""
        toolchain = self.normalize_path(self.config["clangd"]["toolchain_root_path"])
        project_root = self.normalize_path(self.config["clangd"]["project_root"])

        flags: List[str] = [
            f"--gcc-toolchain={toolchain}",
            "--target=arm-none-eabi",
        ]

        # Predefined macros from compiler
        try:
            predefines = self.get_gcc_predefines()
            flags.extend(predefines)
        except Exception as e:
            print(f"Warning: Could not get predefines for fallback flags: {e}", file=sys.stderr)

        # Project definitions
        for macro in self.config["clangd"]["project_definitions"]:
            flags.append(f"-D{macro}")

        # Force-include header (same as template)
        force_include = (
            f"{project_root}/libraries/stm32SL/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/stm32f10x.h"
        )
        flags.extend(["-include", force_include])

        # Project include paths (-I)
        project_includes = [
            f"{project_root}/libraries/MUSSTL/install/include",
            f"{project_root}/libraries/STM32F10x_StdPeriph_Driver/inc",
            f"{project_root}/libraries/stm32SL/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x",
            f"{project_root}/libraries/stm32SL/Libraries/CMSIS/CM3/CoreSupport",
            f"{project_root}/libraries/stm32SL/Libraries",
            project_root,
        ]
        for inc in project_includes:
            flags.extend(["-I", inc])

        # System include paths (-isystem)
        sys_includes = self.get_system_include_paths()
        for sys_inc in sys_includes:
            sys_inc = self.normalize_path(sys_inc)
            flags.extend(["-isystem", sys_inc])

        # Suppress certain diagnostics
        flags.append("-Wno-drv_argument_not_allowed_with")

        # Deduplicate -D flags (keep last occurrence in original order)
        flags = self._deduplicate_defines(flags)

        # Write JSON
        fallback_file = self.output_path.parent / "clangd_fallback_flags.json"
        with open(fallback_file, 'w', encoding='utf-8') as f:
            json.dump(flags, f, indent=2)
        print(f"Generated fallback flags: {fallback_file}")

    @staticmethod
    def _deduplicate_defines(flags: List[str]) -> List[str]:
        """Remove duplicate -D macros, keeping the last occurrence."""
        seen = set()
        result = []
        for f in reversed(flags):
            if f.startswith("-D"):
                # Extract macro name (up to '=' or end)
                macro_name = f[2:].split('=')[0]
                if macro_name not in seen:
                    seen.add(macro_name)
                    result.append(f)
            else:
                result.append(f)
        result.reverse()
        return result


# -----------------------------------------------------------------------------
# Main entry point
# -----------------------------------------------------------------------------
def main():
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    config_file = project_root / "clangd_config.json"
    template_file = project_root / ".clangd.template.yml"
    output_file = project_root / ".clangd.test"   # change to .clangd when ready

    # Allow overrides via environment variables
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