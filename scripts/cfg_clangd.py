#!/usr/bin/env python3
"""
Automatically generate .clang-tidy and .clangd configuration files
Adapt configuration based on different development environments
"""

import os
import sys
import subprocess
from pathlib import Path
from typing import Dict, List, Any


class ClangConfigGenerator:
    def __init__(self, toolchain_path: str, target_mcu: str = "cortex-m3"):
        self.toolchain_path = Path(toolchain_path)
        self.target_mcu = target_mcu
        self.config = self._build_config()

    def _build_config(self) -> Dict[str, Any]:
        """Build configuration dictionary"""
        # Suppressed warnings list
        suppress_list = [
        'unknown-argument',
        'drv_argument_not_allowed_with',
        'asm_naked_parm_ref',
        'non_asm_stmt_in_naked_function',
        'embedded_include_in_asm',
        'expected_string_literal_in_asm'
    ]
    
        # 获取查询驱动路径
        query_driver = self._get_query_driver_path()
    
        return {
        # Toolchain related
        "TOOLCHAIN_PATH": str(self.toolchain_path),
        "TARGET_MCU": self.target_mcu,
        "COMPILER_PATH": query_driver,  # 修改键名为 COMPILER_PATH
        
        # Naming conventions
        "STRUCT_CASE": "camelBack",
        "CLASS_CASE": "CamelCase", 
        "FUNCTION_CASE": "camelBack",
        "VARIABLE_CASE": "camelBack",
        "MEMBER_CASE": "camelBack",
        "CONSTANT_CASE": "UPPER_CASE",
        
        # Suppressed warnings list (formatted string)
        "SUPPRESS_LIST_TIDY": self._format_suppress_list(suppress_list, 2),
        "SUPPRESS_LIST_CLANGD": self._format_suppress_list(suppress_list, 4),
        
        # Compilation database
        "COMPILATION_DATABASE": "build",
        
        # Index settings
        "INDEX_BACKGROUND": "Build",
        
        # Hover settings
        "SHOW_AKA": "false",
        
        # Compilation flags (formatted string)
        "COMPILE_FLAGS": self._get_compile_flags_string(),
        
        # Removed flags (formatted string)
        "REMOVE_FLAGS": self._get_remove_flags_string()
    }

    def _get_query_driver_path(self) -> str:
        """获取查询驱动路径"""
        # 尝试查找编译器路径
        gcc_path = self.toolchain_path / "bin" / "arm-none-eabi-gcc"
        if not gcc_path.exists():
            # 尝试 Windows 扩展名
            gcc_path = self.toolchain_path / "bin" / "arm-none-eabi-gcc.exe"
            if not gcc_path.exists():
                # 如果找不到，返回空字符串
                return ""

        # 返回规范化路径（使用正斜杠）
        return str(gcc_path.resolve()).replace('\\', '/')

    def _format_suppress_list(self, suppress_list: List[str], indent: int) -> str:
        """Format suppress list"""
        indent_str = " " * indent
        return "\n".join([f'{indent_str}- "{item}"' for item in suppress_list])

    def _get_compile_flags(self) -> List[str]:
        """Get compilation flags list"""
        flags = [
            "--target=arm-none-eabi",
            "-mthumb",
            f"-mcpu={self.target_mcu}",

            # C++ feature restrictions
            "-fno-exceptions",
            "-fno-unwind-tables",
            "-fno-rtti",
            "-fno-use-cxa-atexit",
            "-fno-threadsafe-statics",
            "-fno-sized-deallocation",

            # C++ header
            "-xc++",

            # Optimization and linking options
            "-fdata-sections",
            "-ffunction-sections",
            "-fstack-usage",

            # Project definitions
            "-DUSE_STDPERIPH_DRIVER",
            "-DSTM32F10X_MD",
            "-DOS_STM_RAM_SIZE=0x5000",
            "-D__CORTEX_M3",
            "-D__CPLUSPLUS",
            "-D__cplusplus=201709L",
            "-DNO_EXCEPTIONS",
            "-DNO_RTTI",

            # ARM architecture definitions
            "-D__ARM_ARCH=7",
            "-D__thumb2__=1",
            "-D__THUMBEL__=1",

            # Disable standard library
            "-nostdinc",
            "-nostdinc++",

            # Force include main header
            "-include",
            "stm32f10x.h"
        ]

        # Add toolchain header paths from compiler
        flags.extend(self._get_toolchain_include_paths())

        return flags

    def _get_compile_flags_string(self) -> str:
        """Get formatted compilation flags string"""
        flags = self._get_compile_flags()
        result = []
        i = 0
        while i < len(flags):
            if flags[i].startswith(('-isystem', '-include')) and i + 1 < len(flags):
                # Handle flags with parameters
                result.append(f"    - {flags[i]}")
                result.append(f"    - {flags[i + 1]}")
                i += 2
            else:
                # Handle single flags
                result.append(f"    - {flags[i]}")
                i += 1
        return "\n".join(result)

    def _get_remove_flags_string(self) -> str:
        """Get formatted remove flags string"""
        remove_flags = [
            "-m32",
            "-m64",
            "-stdlib=libc++",
            "-specs=*",
            "-fno-weak",
            "-std=*"
        ]
        return "\n".join([f"    - {flag}" for flag in remove_flags])

    def _query_compiler_for_paths(self) -> List[str]:
        """Query compiler for system include paths and convert to absolute paths"""
        try:
            # Find GCC compiler in toolchain path
            gcc_path = self.toolchain_path / "bin" / "arm-none-eabi-gcc"
            if not gcc_path.exists():
                # Try with .exe extension on Windows
                gcc_path = self.toolchain_path / "bin" / "arm-none-eabi-gcc.exe"
                if not gcc_path.exists():
                    raise FileNotFoundError(f"Compiler not found: {gcc_path}")

            # Use compiler to get system include paths
            cmd = [
                str(gcc_path),
                "-mcpu=cortex-m3",
                "-mthumb",
                "-E",
                "-Wp,-v",
                "-"
            ]

            # Run the command with empty input
            result = subprocess.run(
                cmd,
                input="",
                text=True,
                capture_output=True,
                timeout=30
            )

            # Parse output to find include paths
            include_paths = []
            in_include_section = False

            for line in result.stderr.split('\n'):
                line = line.strip()

                # Look for start of include section
                if "#include <...> search starts here:" in line:
                    in_include_section = True
                    continue

                # Look for end of include section
                if "End of search list." in line:
                    break

                # Collect paths in the include section
                if in_include_section and line:
                    # Convert path to absolute and resolve any relative components
                    raw_path = Path(line)

                    # Handle relative paths by making them absolute relative to toolchain
                    if not raw_path.is_absolute():
                        # If path is relative, assume it's relative to toolchain directory
                        abs_path = (self.toolchain_path / raw_path).resolve()
                    else:
                        # If path is absolute but contains relative components, resolve them
                        abs_path = raw_path.resolve()

                    # Convert to string with forward slashes for consistency
                    path_str = str(abs_path).replace('\\', '/')
                    include_paths.append(path_str)

            if not include_paths:
                raise RuntimeError("Compiler did not return any include paths, please check toolchain configuration")

            return include_paths

        except subprocess.TimeoutExpired:
            raise RuntimeError("Compiler query timeout, please check if toolchain is correctly installed")
        except Exception as e:
            raise RuntimeError(f"Failed to query compiler for include paths: {e}")

    def _find_all_subdirectories(self, base_paths: List[str]) -> List[str]:
        """Recursively find all subdirectories"""
        all_dirs = []

        for base_path_str in base_paths:
            base_path = Path(base_path_str)
            if not base_path.exists():
                print(f"Warning: Base path does not exist, skipping: {base_path}")
                continue

            if not base_path.is_dir():
                print(f"Warning: Path is not a directory, skipping: {base_path}")
                continue

            # Add base path itself
            all_dirs.append(str(base_path).replace('\\', '/'))

            # Recursively traverse all subdirectories
            try:
                for item in base_path.rglob('*'):
                    if item.is_dir():
                        dir_path = str(item).replace('\\', '/')
                        all_dirs.append(dir_path)
            except PermissionError as e:
                print(f"Warning: No permission to access subdirectories of {base_path}: {e}")
            except Exception as e:
                print(f"Warning: Error while traversing {base_path}: {e}")

        return all_dirs

    def _normalize_paths(self, paths: List[str]) -> List[str]:
        """Normalize paths - resolve relative components and use forward slashes"""
        normalized = []
        for path_str in paths:
            try:
                path = Path(path_str)
                # Resolve the path to eliminate any relative components (.., etc.)
                if path.exists():
                    resolved_path = path.resolve()
                else:
                    # If path doesn't exist, try to resolve relative to toolchain
                    if not path.is_absolute():
                        resolved_path = (self.toolchain_path / path).resolve()
                    else:
                        resolved_path = path

                # Convert to string with forward slashes
                normalized_path = str(resolved_path).replace('\\', '/')
                normalized.append(normalized_path)
            except Exception as e:
                print(f"Warning: Could not normalize path {path_str}: {e}")
                # Fallback: just use the original path with forward slashes
                normalized.append(path_str.replace('\\', '/'))

        return normalized

    def _get_toolchain_include_paths(self) -> List[str]:
        """Get toolchain include paths by querying compiler and finding all subdirectories"""
        # Get base include paths from compiler
        base_include_paths = self._query_compiler_for_paths()

        # Normalize base paths
        normalized_base_paths = self._normalize_paths(base_include_paths)

        print(f"Found {len(normalized_base_paths)} base include paths:")
        for path in normalized_base_paths:
            print(f"  - {path}")

        # Recursively find all subdirectories
        all_directories = self._find_all_subdirectories(normalized_base_paths)

        # Remove duplicate paths
        seen = set()
        unique_paths = []
        for path in all_directories:
            if path not in seen:
                seen.add(path)
                unique_paths.append(path)

        print(f"Total found {len(unique_paths)} unique directory paths")

        # Limit output to avoid console flooding
        if len(unique_paths) > 50:
            print("Displaying first 50 directory paths:")
            for path in unique_paths[:50]:
                print(f"  - {path}")
            print(f"... and {len(unique_paths) - 50} more directories")
        else:
            print("All directory paths:")
            for path in unique_paths:
                print(f"  - {path}")

        # Add all paths to compilation flags
        compile_flags = []
        for path in unique_paths:
            compile_flags.extend(["-isystem", path])

        return compile_flags

    def _render_template(self, template_file: Path, output_file: Path) -> None:
        """Render template file"""
        if not template_file.exists():
            raise FileNotFoundError(f"Template file does not exist: {template_file}")

        with open(template_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Replace placeholders
        for key, value in self.config.items():
            placeholder = f"{{{key}}}"
            content = content.replace(placeholder, str(value))

        # Write output file
        output_file.parent.mkdir(parents=True, exist_ok=True)
        with open(output_file, 'w', encoding='utf-8', newline='\n') as f:
            f.write(content)

        print(f"Generated file: {output_file}")

    def generate(self) -> None:
        """Generate all configuration files"""
        current_dir = Path(__file__).parent

        # Generate .clang-tidy
        tidy_template = current_dir / "../.clang-tidy.template"
        tidy_output = current_dir / "../.clang-tidy"
        self._render_template(tidy_template, tidy_output)

        # Generate .clangd
        clangd_template = current_dir / "../.clangd.template"
        clangd_output = current_dir / "../.clangd"
        self._render_template(clangd_template, clangd_output)


def main():
    """Main function"""
    # Check command line arguments
    if len(sys.argv) < 2:
        print("Usage: python generate_clang_config.py <toolchain_path> [target_mcu]")
        print("  toolchain_path: ARM GCC toolchain path")
        print("  target_mcu: Target MCU (default: cortex-m3)")
        sys.exit(1)

    # Get toolchain path from command line
    toolchain_path = sys.argv[1]

    # Get target MCU from command line (optional)
    target_mcu = sys.argv[2] if len(sys.argv) > 2 else "cortex-m3"

    # Validate toolchain path
    toolchain_path_obj = Path(toolchain_path)
    if not toolchain_path_obj.exists():
        raise FileNotFoundError(f"Toolchain path does not exist: {toolchain_path}")

    # Generate configuration
    print("Generating Clang configuration files...")
    generator = ClangConfigGenerator(toolchain_path, target_mcu)
    generator.generate()
    print("Configuration files generated successfully!")


if __name__ == "__main__":
    main()