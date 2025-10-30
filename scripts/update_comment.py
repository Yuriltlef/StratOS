#!/usr/bin/env python3
"""
文件注释自动添加/更新脚本
功能：扫描项目目录，为源代码文件添加或更新标准注释头
"""

import os
import re
import json
import datetime
from pathlib import Path

class FileCommentUpdater:
    def __init__(self, root_dir, config_file="comment_config.json"):
        self.root_dir = Path(root_dir)
        self.config = self.load_config(config_file)
        
        # 模板映射
        self.template_map = {
            "c_template": self._c_comment_template,
            "python_template": self._python_comment_template,
            "script_template": self._script_comment_template
        }
        
        # 正则表达式模式
        self.placeholder_pattern = re.compile(
            r'/\*\*[\s\*]*\*{5,}.*?\*{5,}.*?\*/', 
            re.DOTALL
        )
        self.python_placeholder_pattern = re.compile(
            r'\"\"\"\s*\*{5,}.*?\*{5,}.*?\"\"\"', 
            re.DOTALL
        )
        self.script_placeholder_pattern = re.compile(
            r'#\s*\*{5,}.*?\*{5,}.*?(?=\n[^#]|\Z)', 
            re.DOTALL
        )

    def load_config(self, config_file):
        """加载配置文件"""
        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"警告: 配置文件 {config_file} 未找到，使用默认配置")
            return self.get_default_config()
        except json.JSONDecodeError as e:
            print(f"错误: 配置文件格式错误 - {e}")
            return self.get_default_config()

    def get_default_config(self):
        """获取默认配置"""
        return {
            "exclude_dirs": [".cache", ".vscode", "build", "bin"],
            "exclude_files": ["README.md", "README"],
            "file_descriptions": {
                ".c": {
                    "default": {
                        "description": "STM32标准库驱动源文件",
                        "note": "此文件包含STM32标准库的外设驱动实现"
                    },
                    "specific": {}
                },
                ".h": {
                    "default": {
                        "description": "STM32标准库头文件",
                        "note": "此文件包含STM32标准库的函数声明和宏定义"
                    },
                    "specific": {}
                },
                ".cpp": {
                    "default": {
                        "description": "C++源文件",
                        "note": "此文件包含C++类的实现代码"
                    },
                    "specific": {}
                },
                ".hpp": {
                    "default": {
                        "description": "C++头文件",
                        "note": "此文件包含C++类的声明和模板定义"
                    },
                    "specific": {}
                },
                ".py": {
                    "default": {
                        "description": "Python脚本文件",
                        "note": "此文件包含Python脚本代码"
                    },
                    "specific": {}
                },
                ".cmake": {
                    "default": {
                        "description": "CMake构建脚本",
                        "note": "此文件包含CMake构建系统的配置指令"
                    },
                    "specific": {}
                },
                ".md": {
                    "default": {
                        "description": "项目文档文件",
                        "note": "此文件包含项目使用说明和开发文档"
                    },
                    "specific": {}
                }
            },
            "template_types": {
                "c_template": [".c", ".h", ".cpp", ".hpp"],
                "python_template": [".py"],
                "script_template": [".cmake", ".md"]
            }
        }

    def _c_comment_template(self, filename, description, note):
        """C/C++ 文件注释模板"""
        current_date = datetime.datetime.now().strftime("%d-%B-%Y")
        note_section = f"  * @note    {note}\n" if note else ""
        
        return f"""/**
  ******************************************************************************
  * @file    {filename}
  * @author  Yurilt
  * @version V1.0.0
  * @date    {current_date}
  * @brief   {description}
{note_section}  ******************************************************************************
  * @attention
  *
  * Copyright (c) {datetime.datetime.now().year} Yurilt.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

"""

    def _python_comment_template(self, filename, description, note):
        """Python 文件注释模板"""
        current_date = datetime.datetime.now().strftime("%d-%B-%Y")
        note_section = f"* @note    {note}\n" if note else ""
        
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
* Copyright (c) {datetime.datetime.now().year} Yurilt.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
"""

'''

    def _script_comment_template(self, filename, description, note):
        """脚本文件注释模板"""
        current_date = datetime.datetime.now().strftime("%d-%B-%Y")
        note_section = f"# * @note    {note}\n" if note else ""
        
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
# * Copyright (c) {datetime.datetime.now().year} Yurilt.
# * All rights reserved.
# *
# * This software is licensed under terms that can be found in the LICENSE file
# * in the root directory of this software component.
# * If no LICENSE file comes with this software, it is provided AS-IS.
# *
# ******************************************************************************
#

'''

    def get_file_info(self, file_path):
        """获取文件的描述和note信息"""
        suffix = file_path.suffix.lower()
        filename = file_path.name
        
        # 检查文件类型是否在配置中
        if suffix in self.config["file_descriptions"]:
            file_config = self.config["file_descriptions"][suffix]
            
            # 优先使用特定文件配置，否则使用默认配置
            if filename in file_config["specific"]:
                specific_config = file_config["specific"][filename]
                return specific_config.get("description", ""), specific_config.get("note", "")
            else:
                default_config = file_config["default"]
                return default_config.get("description", ""), default_config.get("note", "")
        
        return "项目文件", ""

    def get_file_template(self, file_path):
        """获取文件的模板函数"""
        suffix = file_path.suffix.lower()
        
        for template_type, extensions in self.config["template_types"].items():
            if suffix in extensions:
                return self.template_map.get(template_type, self._c_comment_template)
        
        return self._c_comment_template

    def should_process_file(self, file_path):
        """判断是否应该处理该文件"""
        # 检查是否在排除目录中
        for part in file_path.parts:
            if part in self.config["exclude_dirs"]:
                return False
        
        # 检查是否是排除文件
        if file_path.name in self.config["exclude_files"]:
            return False
        
        # 检查文件后缀是否在配置中
        suffix = file_path.suffix.lower()
        return suffix in self.config["file_descriptions"]

    def find_files_to_process(self):
        """查找所有需要处理的文件"""
        files_to_process = []
        
        for file_path in self.root_dir.rglob('*'):
            if file_path.is_file() and self.should_process_file(file_path):
                files_to_process.append(file_path)
        
        return files_to_process

    def has_comment_placeholder(self, content, file_suffix):
        """检查文件内容是否包含注释占位符"""
        if file_suffix in ['.c', '.h', '.cpp', '.hpp']:
            return self.placeholder_pattern.search(content) is not None
        elif file_suffix == '.py':
            return self.python_placeholder_pattern.search(content) is not None
        elif file_suffix in ['.cmake', '.md']:
            return self.script_placeholder_pattern.search(content) is not None
        return False

    def remove_existing_placeholder(self, content, file_suffix):
        """完全删除现有的注释占位符"""
        if file_suffix in ['.c', '.h', '.cpp', '.hpp']:
            # 删除C风格注释
            new_content = self.placeholder_pattern.sub('', content)
            # 删除可能的多余空行
            new_content = re.sub(r'^\s*\n', '', new_content, flags=re.MULTILINE)
            return new_content
        elif file_suffix == '.py':
            # 删除Python多行注释
            new_content = self.python_placeholder_pattern.sub('', content)
            # 如果删除后文件以空行开头，删除开头的空行
            new_content = re.sub(r'^\s*\n', '', new_content, flags=re.MULTILINE)
            return new_content
        elif file_suffix in ['.cmake', '.md']:
            # 删除脚本注释
            new_content = self.script_placeholder_pattern.sub('', content)
            new_content = re.sub(r'^\s*\n', '', new_content, flags=re.MULTILINE)
            return new_content
        return content

    def update_file_comments(self, file_path):
        """更新或插入文件注释"""
        try:
            # 读取文件内容（使用UTF-8编码）
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 获取文件后缀
            suffix = file_path.suffix.lower()
            
            # 获取文件描述和note信息
            description, note = self.get_file_info(file_path)
            template_func = self.get_file_template(file_path)
            
            # 生成新的注释
            new_comment = template_func(file_path.name, description, note)
            
            # 检查是否有注释占位符
            if self.has_comment_placeholder(content, suffix):
                # 完全删除现有的注释占位符
                content_without_placeholder = self.remove_existing_placeholder(content, suffix)
                
                # 在文件开头插入新注释
                new_content = new_comment + content_without_placeholder
                
                print(f"  └── 替换注释占位符")
            else:
                # 检查是否已经有类似的注释（避免重复添加）
                if not self._has_similar_comment(content, file_path.name):
                    # 在文件开头插入新注释
                    new_content = new_comment + content
                    note_info = f" [Note: {note}]" if note else ""
                    print(f"  └── 添加新注释 [{description}]{note_info}")
                else:
                    print(f"  └── 已有类似注释，跳过")
                    return True
            
            # 使用UTF-8编码写入文件
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            
            return True
            
        except Exception as e:
            print(f"处理文件 {file_path} 时出错: {e}")
            return False

    def _has_similar_comment(self, content, filename):
        """检查是否已经有类似的注释（避免重复添加）"""
        # 检查常见的注释标识
        patterns = [
            r'@file\s+' + re.escape(filename),
            r'@author\s+Yurilt',
            r'Copyright\s+\(c\)\s+\d{4}\s+Yurilt',
        ]
        
        for pattern in patterns:
            if re.search(pattern, content, re.IGNORECASE):
                return True
        return False

    def process_all_files(self):
        """处理所有文件"""
        print("开始扫描项目文件...")
        files_to_process = self.find_files_to_process()
        
        print(f"找到 {len(files_to_process)} 个需要处理的文件")
        
        success_count = 0
        for file_path in files_to_process:
            relative_path = file_path.relative_to(self.root_dir)
            print(f"处理: {relative_path}")
            
            if self.update_file_comments(file_path):
                success_count += 1
            else:
                print(f"  └── 失败")
        
        print(f"\n处理完成! 成功更新 {success_count}/{len(files_to_process)} 个文件")

def main():
    # 设置项目根目录（当前目录）
    project_root = os.getcwd()
    
    # 配置文件路径
    config_file = "comment_config.json"
    
    # 创建更新器实例
    updater = FileCommentUpdater(project_root, config_file)
    
    # 执行处理
    updater.process_all_files()

if __name__ == "__main__":
    main()