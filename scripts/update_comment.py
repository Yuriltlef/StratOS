#!/usr/bin/env python3
"""
文件注释自动添加/更新脚本
功能：扫描项目目录，为源代码文件添加或更新标准注释头
"""

import os
import json
import datetime
from pathlib import Path

class FileCommentUpdater:
    def __init__(self, root_dir, config_file="comment_config.json"):
        self.root_dir = Path(root_dir)
        self.config = self.load_config(config_file)
        
        # 注释符号定义
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
            }
        }

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

    def generate_comment(self, filename, file_ext, description, note):
        """生成注释内容"""
        current_date = datetime.datetime.now().strftime("%d-%B-%Y")
        year = datetime.datetime.now().year
        
        # 添加note字段（如果存在）
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
        """处理单行的栈操作"""
        i = 0
        line_len = len(line)
        
        while i < line_len:
            if s_sym == e_sym:
                # 对于起始和结束符号相同的情况（如Python的三引号）
                if i + len(s_sym) <= line_len and line[i:i+len(s_sym)] == s_sym:
                    if not stack:  # 栈为空，压入
                        stack.append(s_sym)
                        i += len(s_sym)
                    else:  # 栈不为空，弹出
                        stack.pop()
                        i += len(s_sym)
                else:
                    i += 1
            else:
                # 对于起始和结束符号不同的情况（如C的/*和*/）
                if i + len(s_sym) <= line_len and line[i:i+len(s_sym)] == s_sym:
                    stack.append(s_sym)
                    i += len(s_sym)
                elif i + len(e_sym) <= line_len and line[i:i+len(e_sym)] == e_sym:
                    if not stack:
                        raise Exception("意外的结束符号")
                    stack.pop()
                    i += len(e_sym)
                else:
                    i += 1
        
        return stack

    def interval(self, content, file_ext):
        """找到注释块的起始和结束位置，匹配错误时抛出异常"""
        if file_ext not in self.comment_symbols:
            return 0, 0

        s_sym, e_sym = self.comment_symbols[file_ext]
        lines = content.split('\n')

        if not lines:
            return 0, 0

        # 检查第一行是否有起始符号
        first_line = lines[0].strip()

        # 对于脚本文件（#注释），我们检查是否以注释开始
        if file_ext in ['.cmake', '.md']:
            if first_line.startswith(s_sym):
                # 找到连续的注释行
                end_line = 0
                for i, line in enumerate(lines):
                    if not line.strip().startswith(s_sym):
                        end_line = i
                        break
                else:
                    end_line = len(lines)
                return 0, end_line
            return 0, 0

        # 对于C和Python文件，使用栈匹配
        stack = []

        # 检查第一行
        stack = self.match_line(lines[0], s_sym, e_sym, stack)

        if not stack:
            # 栈为空，说明第一行就完成了匹配（单行注释）
            # 检查第一行结束符号后是否有非空白字符
            if file_ext in ['.c', '.h', '.cpp', '.hpp']:
                end_pos = lines[0].find(e_sym)
                if end_pos != -1:
                    after_comment = lines[0][end_pos + len(e_sym):]
                    if after_comment.strip():  # 有非空白字符
                        raise Exception("注释结束符号后存在非空白字符")
            elif file_ext == '.py':
                end_pos = lines[0].find(e_sym)
                if end_pos != -1:
                    after_comment = lines[0][end_pos + len(e_sym):]
                    if after_comment.strip():  # 有非空白字符
                        raise Exception("注释结束符号后存在非空白字符")

            return 0, 1

        # 继续处理后续行
        e = 0
        for i in range(1, len(lines)):
            e = i
            stack = self.match_line(lines[i], s_sym, e_sym, stack)

            if not stack:
                # 栈为空，匹配完成
                # 检查结束行结束符号后是否有非空白字符
                if file_ext in ['.c', '.h', '.cpp', '.hpp']:
                    end_pos = lines[i].find(e_sym)
                    if end_pos != -1:
                        after_comment = lines[i][end_pos + len(e_sym):]
                        if after_comment.strip():  # 有非空白字符
                            raise Exception("注释结束符号后存在非空白字符")
                elif file_ext == '.py':
                    end_pos = lines[i].find(e_sym)
                    if end_pos != -1:
                        after_comment = lines[i][end_pos + len(e_sym):]
                        if after_comment.strip():  # 有非空白字符
                            raise Exception("注释结束符号后存在非空白字符")

                return 0, e + 1

        # 如果处理完所有行栈还不为空，说明注释不完整
        if stack:
            raise Exception("注释块不完整，缺少结束符号")

        return 0, e + 1

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

    def update_file_comments(self, file_path):
        """更新或插入文件注释"""
        try:
            # 读取文件内容
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 获取文件后缀和描述
            file_ext = file_path.suffix.lower()
            description, note = self.get_file_info(file_path)
            
            # 生成新注释
            new_comment = self.generate_comment(file_path.name, file_ext, description, note)
            
            # 找到注释区间
            s, e = self.interval(content, file_ext)
            
            if e > 0:
                # 替换现有注释
                lines = content.split('\n')
                remaining_content = '\n'.join(lines[e:])
                
                # 确保注释和内容之间有适当的空行
                if remaining_content and not remaining_content.startswith('\n'):
                    new_content = new_comment + '\n' + remaining_content
                else:
                    new_content = new_comment + remaining_content
                    
                print(f"  └── 替换注释 (第1-{e}行)")
            else:
                # 插入新注释
                if content and not content.startswith('\n'):
                    new_content = new_comment + '\n' + content
                else:
                    new_content = new_comment + content
                print(f"  └── 添加新注释")
            
            # 写入文件
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            
            return True
            
        except Exception as e:
            print(f"处理文件 {file_path} 时出错: {e}")
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