#!/usr/bin/env python3
"""
Automatically generate .clang-tidy and .clangd configuration files
Adapt configuration based on different development environments
"""

import os
import sys
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
        
        return {
            # Toolchain related
            "TOOLCHAIN_PATH": str(self.toolchain_path),
            "TARGET_MCU": self.target_mcu,
            
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
        
        # Add toolchain header paths
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
                result.append(f"    - {flags[i+1]}")
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
    
    def _get_toolchain_include_paths(self) -> List[str]:
        """Get toolchain include paths"""
        paths = []
        base_path = self.toolchain_path
        
        # Main include paths
        include_paths = [
            base_path / "arm-none-eabi/include",
            base_path / "arm-none-eabi/include/c++/10.3.1",
            base_path / "arm-none-eabi/include/c++/10.3.1/arm-none-eabi",
            base_path / "lib/gcc/arm-none-eabi/10.3.1/include",
            base_path / "lib/gcc/arm-none-eabi/10.3.1/include-fixed"
        ]
        
        # Architecture specific paths
        arch_paths = [
            "thumb", "thumb/nofp", "thumb/v6-m", "thumb/v7", "thumb/v7+fp",
            "thumb/v7-a", "thumb/v7-a+fp", "thumb/v7-a+simd", "thumb/v7-m",
            "thumb/v7-r+fp.sp", "thumb/v7e-m", "thumb/v7e-m+dp", "thumb/v7e-m+fp",
            "thumb/v7ve+simd", "thumb/v8-a", "thumb/v8-a+simd", "thumb/v8-m.base",
            "thumb/v8-m.main", "thumb/v8-m.main+dp", "thumb/v8-m.main+fp",
            "thumb/v8.1-m.main+mve"
        ]
        
        for arch in arch_paths:
            arch_path = base_path / f"arm-none-eabi/include/c++/10.3.1/arm-none-eabi/{arch}"
            if arch_path.exists():
                include_paths.append(arch_path)
        
        # Add all paths to compilation flags
        compile_flags = []
        for path in include_paths:
            if path.exists():
                compile_flags.extend(["-isystem", str(path)])
        
        return compile_flags
    
    def _render_template(self, template_file: Path, output_file: Path) -> None:
        """Render template file"""
        if not template_file.exists():
            print(f"Error: Template file {template_file} does not exist")
            return
        
        with open(template_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Replace placeholders
        for key, value in self.config.items():
            placeholder = f"{{{key}}}"
            content = content.replace(placeholder, str(value))
        
        # Write output file
        with open(output_file, 'w', encoding='utf-8', newline='\n') as f:
            f.write(content)
        
        print(f"Generated: {output_file}")
    
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
        print("  toolchain_path: Path to ARM GCC toolchain")
        print("  target_mcu: Target MCU (default: cortex-m3)")
        sys.exit(1)
    
    # Get toolchain path from command line
    toolchain_path = sys.argv[1]
    
    # Get target MCU from command line (optional)
    target_mcu = sys.argv[2] if len(sys.argv) > 2 else "cortex-m3"
    
    # Validate toolchain path
    toolchain_path_obj = Path(toolchain_path)
    if not toolchain_path_obj.exists():
        print(f"Error: Toolchain path does not exist: {toolchain_path}")
        raise NotADirectoryError(f"Toolchain path does not exist: {toolchain_path}")
    
    # Generate configuration
    print("Generating Clang configuration files...")
    generator = ClangConfigGenerator(toolchain_path, target_mcu)
    generator.generate()
    print("Configuration files generated successfully!")

if __name__ == "__main__":
    main()