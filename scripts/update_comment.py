#!/usr/bin/env python3
"""
File Comment Auto-Update Script
Function: Scan project directory, add or update standard comment headers for source code files
"""

import os
import json
import datetime
import sys
from pathlib import Path

class FileCommentUpdater:
    def __init__(self, root_dir, config_file="comment_config.json"):
        self.root_dir = Path(root_dir)
        self.config = self.load_config(config_file)
        
        # Comment symbols definition
        self.comment_symbols = {
            '.c': ('/*', '*/'),
            '.h': ('/*', '*/'),
            '.cpp': ('/*', '*/'),
            '.hpp': ('/*', '*/'),
            '.py': ('"""', '"""'),
            '.cmake': ('#', '#'),
            '.md': ('#', '#')
        }

    def load_config(self, config_file):
        """Load configuration file"""
        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"WARNING: Configuration file {config_file} not found, using default configuration")
            return self.get_default_config()
        except json.JSONDecodeError as e:
            print(f"ERROR: Configuration file format error - {e}")
            return self.get_default_config()

    def get_default_config(self):
        """Get default configuration"""
        return {
            "exclude_dirs": [".cache", ".vscode", "build", "bin"],
            "exclude_files": ["README.md", "README"],
            "file_descriptions": {
                ".c": {
                    "default": {
                        "description": "STM32 Standard Library Driver Source File",
                        "note": "This file contains STM32 standard library peripheral driver implementation"
                    },
                    "specific": {}
                },
                ".h": {
                    "default": {
                        "description": "STM32 Standard Library Header File",
                        "note": "This file contains STM32 standard library function declarations and macro definitions"
                    },
                    "specific": {}
                },
                ".cpp": {
                    "default": {
                        "description": "C++ Source File", 
                        "note": "This file contains C++ class implementation code"
                    },
                    "specific": {}
                },
                ".hpp": {
                    "default": {
                        "description": "C++ Header File",
                        "note": "This file contains C++ class declarations and template definitions"
                    },
                    "specific": {}
                },
                ".py": {
                    "default": {
                        "description": "Python Script File",
                        "note": "This file contains Python script code"
                    },
                    "specific": {}
                },
                ".cmake": {
                    "default": {
                        "description": "CMake Build Script",
                        "note": "This file contains CMake build system configuration instructions"
                    },
                    "specific": {}
                },
                ".md": {
                    "default": {
                        "description": "Project Documentation File", 
                        "note": "This file contains project usage instructions and development documentation"
                    },
                    "specific": {}
                }
            }
        }

    def get_file_info(self, file_path):
        """Get file description and note information"""
        suffix = file_path.suffix.lower()
        filename = file_path.name
        
        # Check if file type is in configuration
        if suffix in self.config["file_descriptions"]:
            file_config = self.config["file_descriptions"][suffix]
            
            # Prefer specific file configuration, otherwise use default
            if filename in file_config["specific"]:
                specific_config = file_config["specific"][filename]
                return specific_config.get("description", ""), specific_config.get("note", "")
            else:
                default_config = file_config["default"]
                return default_config.get("description", ""), default_config.get("note", "")
        
        return "Project File", ""

    def generate_comment(self, filename, file_ext, description, note):
        """Generate comment content"""
        current_date = datetime.datetime.now().strftime("%d-%B-%Y")
        year = datetime.datetime.now().year
        
        # Add note field (if exists)
        note_section = ""
        if note:
            if file_ext in ['.c', '.h', '.cpp', '.hpp']:
                note_section = f" * @note    {note}\n"
            elif file_ext == '.py':
                note_section = f"* @note    {note}\n"
            elif file_ext in ['.cmake', '.md']:
                note_section = f"# * @note    {note}\n"
        
        if file_ext in ['.c', '.h', '.cpp', '.hpp']:
            return f"""/**
 ******************************************************************************
 * @file    {filename}
 * @author  Yurilt
 * @version V1.0.0
 * @date    {current_date}
 * @brief   {description}
{note_section} ******************************************************************************
 * @attention
 *
 * Copyright (c) {year} Yurilt.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
"""
        elif file_ext == '.py':
            return f'''#!/usr/bin/env python3
"""
******************************************************************************
* @file    {filename}
* @author  Yurilt
* @version V1.0.0
* @date    {current_date}
* @brief   {description}
{note_section}******************************************************************************
* @attention
*
* Copyright (c) {year} Yurilt.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
"""
'''
        elif file_ext in ['.cmake', '.md']:
            return f'''#
# ******************************************************************************
# * @file    {filename}
# * @author  Yurilt
# * @version V1.0.0
# * @date    {current_date}
# * @brief   {description}
{note_section}# ******************************************************************************
# * @attention
# *
# * Copyright (c) {year} Yurilt.
# * All rights reserved.
# *
# * This software is licensed under terms that can be found in the LICENSE file
# * in the root directory of this software component.
# * If no LICENSE file comes with this software, it is provided AS-IS.
# *
# ******************************************************************************
#
'''

    def match_line(self, line, s_sym, e_sym, stack):
        """Process stack operations for a single line"""
        i = 0
        line_len = len(line)
        
        while i < line_len:
            if s_sym == e_sym:
                # For cases where start and end symbols are the same (like Python triple quotes)
                if i + len(s_sym) <= line_len and line[i:i+len(s_sym)] == s_sym:
                    if not stack:  # Stack is empty, push
                        stack.append(s_sym)
                        i += len(s_sym)
                    else:  # Stack is not empty, pop
                        stack.pop()
                        i += len(s_sym)
                else:
                    i += 1
            else:
                # For cases where start and end symbols are different (like C /* and */)
                if i + len(s_sym) <= line_len and line[i:i+len(s_sym)] == s_sym:
                    stack.append(s_sym)
                    i += len(s_sym)
                elif i + len(e_sym) <= line_len and line[i:i+len(e_sym)] == e_sym:
                    if not stack:
                        raise Exception("Unexpected end symbol")
                    stack.pop()
                    i += len(e_sym)
                else:
                    i += 1
        
        return stack

    def interval(self, content, file_ext):
        """Find the start and end positions of comment block, throw exception on matching error"""
        if file_ext not in self.comment_symbols:
            return 0, 0

        s_sym, e_sym = self.comment_symbols[file_ext]
        lines = content.split('\n')

        if not lines:
            return 0, 0

        # Check if first line has start symbol
        first_line = lines[0].strip()

        # For script files (# comments), check if they start with comments
        if file_ext in ['.cmake', '.md']:
            if first_line.startswith(s_sym):
                # Find consecutive comment lines
                end_line = 0
                for i, line in enumerate(lines):
                    if not line.strip().startswith(s_sym):
                        end_line = i
                        break
                else:
                    end_line = len(lines)
                return 0, end_line
            return 0, 0

        # For C and Python files, use stack matching
        stack = []

        # Check first line
        stack = self.match_line(lines[0], s_sym, e_sym, stack)

        if not stack:
            # Stack is empty, matching completed in first line (single line comment)
            # Check if there are non-whitespace characters after end symbol in first line
            if file_ext in ['.c', '.h', '.cpp', '.hpp']:
                end_pos = lines[0].find(e_sym)
                if end_pos != -1:
                    after_comment = lines[0][end_pos + len(e_sym):]
                    if after_comment.strip():  # Has non-whitespace characters
                        raise Exception("Non-whitespace characters found after comment end symbol")
            elif file_ext == '.py':
                end_pos = lines[0].find(e_sym)
                if end_pos != -1:
                    after_comment = lines[0][end_pos + len(e_sym):]
                    if after_comment.strip():  # Has non-whitespace characters
                        raise Exception("Non-whitespace characters found after comment end symbol")

            return 0, 1

        # Continue processing subsequent lines
        e = 0
        for i in range(1, len(lines)):
            e = i
            stack = self.match_line(lines[i], s_sym, e_sym, stack)

            if not stack:
                # Stack is empty, matching completed
                # Check if there are non-whitespace characters after end symbol in end line
                if file_ext in ['.c', '.h', '.cpp', '.hpp']:
                    end_pos = lines[i].find(e_sym)
                    if end_pos != -1:
                        after_comment = lines[i][end_pos + len(e_sym):]
                        if after_comment.strip():  # Has non-whitespace characters
                            raise Exception("Non-whitespace characters found after comment end symbol")
                elif file_ext == '.py':
                    end_pos = lines[i].find(e_sym)
                    if end_pos != -1:
                        after_comment = lines[i][end_pos + len(e_sym):]
                        if after_comment.strip():  # Has non-whitespace characters
                            raise Exception("Non-whitespace characters found after comment end symbol")

                return 0, e + 1

        # If stack is not empty after processing all lines, comment is incomplete
        if stack:
            raise Exception("Incomplete comment block, missing end symbol")

        return 0, e + 1

    def should_process_file(self, file_path):
        """Determine if the file should be processed"""
        # Check if in excluded directories
        for part in file_path.parts:
            if part in self.config["exclude_dirs"]:
                return False
        
        # Check if excluded file
        if file_path.name in self.config["exclude_files"]:
            return False
        
        # Check if file extension is in configuration
        suffix = file_path.suffix.lower()
        return suffix in self.config["file_descriptions"]

    def find_files_to_process(self):
        """Find all files to process"""
        files_to_process = []
        
        for file_path in self.root_dir.rglob('*'):
            if file_path.is_file() and self.should_process_file(file_path):
                files_to_process.append(file_path)
        
        return files_to_process

    def print_error(self, message):
        """Print error message with emphasis"""
        print(f"\033[91mERROR: {message}\033[0m", file=sys.stderr)

    def print_warning(self, message):
        """Print warning message"""
        print(f"\033[93mWARNING: {message}\033[0m", file=sys.stderr)

    def print_success(self, message):
        """Print success message"""
        print(f"\033[92mSUCCESS: {message}\033[0m")

    def update_file_comments(self, file_path):
        """Update or insert file comments"""
        try:
            # Read file content
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Get file extension and description
            file_ext = file_path.suffix.lower()
            description, note = self.get_file_info(file_path)
            
            # Generate new comment
            new_comment = self.generate_comment(file_path.name, file_ext, description, note)
            
            # Find comment interval
            s, e = self.interval(content, file_ext)
            
            if e > 0:
                # Replace existing comment
                lines = content.split('\n')
                remaining_content = '\n'.join(lines[e:])
                
                # Ensure proper spacing between comment and content
                if remaining_content and not remaining_content.startswith('\n'):
                    new_content = new_comment + '\n' + remaining_content
                else:
                    new_content = new_comment + remaining_content
                    
                print(f"  └── Replaced comment (lines 1-{e})")
            else:
                # Insert new comment
                if content and not content.startswith('\n'):
                    new_content = new_comment + '\n' + content
                else:
                    new_content = new_comment + content
                print(f"  └── Added new comment")
            
            # Write to file
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            
            return True
            
        except Exception as e:
            self.print_error(f"Failed to process file {file_path}: {e}")
            return False

    def process_all_files(self):
        """Process all files"""
        print("Scanning project files...")
        files_to_process = self.find_files_to_process()
        
        print(f"Found {len(files_to_process)} files to process")
        
        success_count = 0
        for file_path in files_to_process:
            relative_path = file_path.relative_to(self.root_dir)
            print(f"Processing: {relative_path}")
            
            if self.update_file_comments(file_path):
                success_count += 1
        
        if success_count == len(files_to_process):
            self.print_success(f"Processing completed! Successfully updated {success_count}/{len(files_to_process)} files")
        else:
            self.print_warning(f"Processing completed with issues. Updated {success_count}/{len(files_to_process)} files")

def main():
    # Set project root directory (current directory)
    project_root = os.getcwd()
    
    # Configuration file path
    config_file = "comment_config.json"
    
    # Create updater instance
    updater = FileCommentUpdater(project_root, config_file)
    
    # Execute processing
    updater.process_all_files()

if __name__ == "__main__":
    main()