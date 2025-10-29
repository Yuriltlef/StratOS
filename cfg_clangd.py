import os
import subprocess
import sys
import glob
from pathlib import Path

def find_arm_gcc_path():
    """查找 ARM GCC 安装路径"""
    possible_paths = [
        "D:\\arm_gcc\\10 2021.10",
        "C:\\Program Files\\ARM\\*",
        "C:\\Program Files (x86)\\GNU Tools ARM Embedded\\*",
        os.environ.get("ARM_GCC_PATH", ""),
        os.environ.get("GCC_ARM_PATH", ""),
    ]
    
    for path_pattern in possible_paths:
        if path_pattern:
            matches = glob.glob(path_pattern)
            for match in matches:
                if os.path.isdir(match):
                    gcc_exe = os.path.join(match, "bin", "arm-none-eabi-gcc")
                    if os.path.exists(gcc_exe) or os.path.exists(gcc_exe + ".exe"):
                        return match
    return None

def get_include_paths_absolute(arm_gcc_path):
    """获取所有包含路径并转换为绝对路径"""
    gcc_exe = os.path.join(arm_gcc_path, "bin", "arm-none-eabi-gcc")
    if os.path.exists(gcc_exe + ".exe"):
        gcc_exe += ".exe"
    
    # 获取 C 和 C++ 的包含路径
    all_paths = []
    
    for lang in ["c", "c++"]:
        cmd = [gcc_exe, "-mthumb", "-mcpu=cortex-m3"]
        if lang == "c++":
            cmd.extend(["-x", "c++"])
        else:
            cmd.extend(["-x", "c"])
        
        cmd.extend(["-E", "-Wp,-v", "-"])
        
        try:
            result = subprocess.run(cmd, 
                                  input="",
                                  capture_output=True, 
                                  text=True,
                                  check=False)
            
            in_include_section = False
            for line in result.stderr.split('\n'):
                if "search starts here" in line:
                    in_include_section = True
                    continue
                elif "End of search list" in line:
                    break
                elif in_include_section and line.strip():
                    path = line.strip().lstrip(' ')
                    if path:
                        # 转换为绝对路径
                        abs_path = os.path.abspath(os.path.join(os.path.dirname(gcc_exe), path))
                        abs_path = abs_path.replace('\\', '/')
                        if os.path.isdir(abs_path) and abs_path not in all_paths:
                            all_paths.append(abs_path)
            
        except Exception as e:
            print(f"Error getting {lang} include paths: {e}", file=sys.stderr)
    
    return all_paths

def get_all_possible_paths(arm_gcc_path):
    """手动构建所有可能的包含路径"""
    base_path = Path(arm_gcc_path)
    possible_paths = [
        # C 标准库路径
        base_path / "arm-none-eabi" / "include",
        base_path / "lib" / "gcc" / "arm-none-eabi" / "*" / "include",
        base_path / "lib" / "gcc" / "arm-none-eabi" / "*" / "include-fixed",
        # C++ 标准库路径
        base_path / "arm-none-eabi" / "include" / "c++" / "*",
        base_path / "arm-none-eabi" / "include" / "c++" / "*" / "arm-none-eabi",
        base_path / "arm-none-eabi" / "include" / "c++" / "*" / "arm-none-eabi" / "thumb",
        base_path / "arm-none-eabi" / "include" / "c++" / "*" / "arm-none-eabi" / "thumb" / "*",
        base_path / "arm-none-eabi" / "include" / "c++" / "*" / "backward",
    ]
    
    all_paths = []
    for path_pattern in possible_paths:
        matches = glob.glob(str(path_pattern))
        for match in matches:
            abs_path = Path(match).resolve()
            if abs_path.is_dir() and str(abs_path) not in all_paths:
                all_paths.append(str(abs_path).replace('\\', '/'))
    
    return sorted(all_paths)

def generate_clangd_config(paths):
    """生成 clangd 配置"""
    config_lines = []
    
    for path in sorted(paths):  # 排序以便更好的可读性
        config_lines.append(f"    - -isystem")
        config_lines.append(f'    - "{path}"')
    
    return "\n".join(config_lines)

def main():
    print("ARM GCC Include Path Scanner (绝对路径版本)")
    print("=" * 60)
    
    # 查找 ARM GCC 路径
    arm_gcc_path = find_arm_gcc_path()
    
    if not arm_gcc_path:
        print("错误: 未找到 ARM GCC 工具链")
        print("请手动指定路径: python script.py <arm_gcc_path>")
        return 1
    
    print(f"找到 ARM GCC: {arm_gcc_path}")
    
    # 方法1: 使用 GCC 输出并转换为绝对路径
    print("\n方法1: 从 GCC 获取路径并转换为绝对路径...")
    gcc_paths = get_include_paths_absolute(arm_gcc_path)
    
    # 方法2: 手动构建所有可能的路径
    print("方法2: 手动构建所有可能的路径...")
    manual_paths = get_all_possible_paths(arm_gcc_path)
    
    # 合并并去重
    all_paths = list(set(gcc_paths + manual_paths))
    
    if not all_paths:
        print("错误: 无法获取包含路径")
        return 1
    
    print(f"\n找到 {len(all_paths)} 个包含路径:")
    for path in sorted(all_paths):
        print(f"  ✓ {path}")
    
    # 生成配置
    print("\n" + "=" * 60)
    print("生成的 -isystem 配置 (复制到 .clangd 文件的 CompileFlags.Add 部分):")
    print("=" * 60)
    
    config = generate_clangd_config(all_paths)
    print(config)
    
    # 保存到文件
    output_file = "arm_gcc_includes_absolute.txt"
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(config)
    
    print(f"\n配置已保存到: {output_file}")
    
    # 验证路径是否存在
    print(f"\n路径验证:")
    missing_paths = []
    for path in all_paths:
        if os.path.isdir(path):
            print(f"  ✓ {path}")
        else:
            print(f"  ✗ {path} (不存在)")
            missing_paths.append(path)
    
    if missing_paths:
        print(f"\n警告: 找到 {len(missing_paths)} 个不存在的路径")
    
    return 0

if __name__ == "__main__":
    if len(sys.argv) > 1:
        custom_path = sys.argv[1]
        if os.path.isdir(custom_path):
            arm_gcc_path = custom_path
            all_paths = get_all_possible_paths(arm_gcc_path)
            if all_paths:
                config = generate_clangd_config(all_paths)
                print(config)
            else:
                print(f"错误: 无法从 {custom_path} 获取包含路径")
        else:
            print(f"错误: 路径不存在: {custom_path}")
    else:
        sys.exit(main())