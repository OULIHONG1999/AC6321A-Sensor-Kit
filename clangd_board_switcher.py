#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# 板级 / clangd 与 AC63 快速编译、清理逻辑均在本文件，可单独拷贝到 SDK 根目录使用（开源，与原 build_fast.ps1、clean_build.ps1 行为对齐）。
import os
import sys
import json
import re
import glob
import time
import argparse
import platform
import shutil
import subprocess

def find_cbp_file(target):
    """
    根据目标查找对应的 .cbp 文件
    """
    board_map = {
        'ac632n': 'bd19',
        'ac638n': 'br34',
        'ac631n': 'bd29',
        'ac636n': 'br25',
        'ac637n': 'br30',
        'ac635n': 'br23'
    }

    for prefix, board_dir in board_map.items():
        if target.startswith(prefix):
            # 尝试查找不同的应用目录
            for app_dir in ['spp_and_le', 'hid', 'mesh']:
                cbp_path = f'apps/{app_dir}/board/{board_dir}/{target}.cbp'
                if os.path.exists(cbp_path):
                    return cbp_path, board_dir
            # 如果没找到精确匹配的，尝试通配
            for app_dir in ['spp_and_le', 'hid', 'mesh']:
                board_path = f'apps/{app_dir}/board/{board_dir}'
                if os.path.exists(board_path):
                    for f in os.listdir(board_path):
                        if f.endswith('.cbp'):
                            return os.path.join(board_path, f), board_dir
    return None, None

def parse_cbp_file(cbp_path):
    """
    解析 .cbp 文件，提取编译选项和源文件列表
    """
    with open(cbp_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 提取编译器选项
    options = []
    for match in re.finditer(r'<Add option="([^"]+)" />', content):
        option = match.group(1)
        if option.startswith('-D'):
            options.append(option)
        elif option.startswith('-I'):
            options.append(option)

    # 提取所有包含目录
    includes = []
    for match in re.finditer(r'<Add directory="([^"]+)" />', content):
        inc = match.group(1)
        # 转换为绝对路径或正确的相对路径
        inc = inc.replace('../../../../', '')
        if not inc.startswith('-'):
            includes.append(f'-I{inc}')

    # 提取源文件列表
    source_files = []
    for match in re.finditer(r'<Unit filename="([^"]+\.c)"', content):
        source_file = match.group(1)
        source_file = source_file.replace('../../../../', '')
        source_files.append(source_file)

    # 提取宏定义
    defines = []
    for match in re.finditer(r'<Add option="(-D[^"]+)" />', content):
        defines.append(match.group(1))

    return defines, includes, source_files


def parse_active_config_board_symbols(board_config_path):
    """
    从 board_config.h 读取当前「未注释」的 #define CONFIG_BOARD_*，
    与真实编译时由该头文件定义的板型一致（.cbp 往往不带这些宏）。
    """
    if not os.path.isfile(board_config_path):
        return []
    out = []
    with open(board_config_path, 'r', encoding='utf-8', errors='replace') as f:
        for raw in f:
            line = raw.split('//', 1)[0].strip()
            if not line or line.startswith('/*') or line.startswith('*'):
                continue
            m = re.match(r'#define\s+(CONFIG_BOARD_[A-Za-z0-9_]+)\b', line)
            if m:
                out.append(m.group(1))
    return out


def board_config_define_flags(board_config_path):
    """转为 clang 命令行片段，如 ['-DCONFIG_BOARD_AC632N_DEMO']。"""
    return [f'-D{name}' for name in parse_active_config_board_symbols(board_config_path)]


def generate_compile_commands(target):
    """
    为指定的板级目标生成 compile_commands.json 文件
    """
    print(f"\nGenerating compile_commands.json for target: {target}")

    # 查找 .cbp 文件
    cbp_path, board_dir = find_cbp_file(target)
    if not cbp_path:
        print(f"No .cbp file found for target: {target}")
        return False

    print(f"Using .cbp file: {cbp_path}")

    cbp_dir = os.path.dirname(cbp_path)
    board_cfg = os.path.join(cbp_dir, 'board_config.h')
    board_flags = board_config_define_flags(board_cfg)
    if board_flags:
        shown = ', '.join(x[2:] for x in board_flags)
        print(f"board_config.h 活动板宏（并入 clangd / compile_commands）: {shown}")
    else:
        print(f"（未在 {board_cfg} 中解析到活动的 CONFIG_BOARD_* 宏）")

    # 先写入 VSCode/clangd 配置（含 onConfigChanged: restart），再更新 compile_commands，
    # 以便 vscode-clangd 在检测到 compile_commands.json 变化时自动重启 clangd，无需重载整个 IDE。
    generate_vscode_config()
    generate_clangd_config(extra_add_compile_flags=board_flags)

    if os.path.exists('compile_commands.json'):
        os.remove('compile_commands.json')

    # 解析 .cbp 文件
    defines, includes, source_files = parse_cbp_file(cbp_path)

    print(f"Found {len(source_files)} source files")
    print(f"Found {len(defines)} macro definitions")
    print(f"Found {len(includes)} include paths")

    # 构建编译命令
    compile_commands = []
    compiler = 'C:/JL/pi32/bin/clang.exe'

    # 基础选项
    base_options = [
        '-c',
        '-integrated-as',
        '-g',
        '-O0',
        '-flto',
        '-Os',
        '-Wcast-align',
        '-w',
        '-fallow-pointer-null',
        '-Wincompatible-pointer-types',
        '-Wundef',
        '-fprefer-gnu-section',
        '-Wframe-larger-than=256',
        '-Wreturn-type',
        '-Wimplicit-function-declaration',
        '-fms-extensions',
        '-DSUPPORT_MS_EXTENSIONS',
        '-DCONFIG_RELEASE_ENABLE',
    ]

    # 添加从 .cbp 提取的宏定义，以及 board_config.h 中的当前板型（否则头文件里 #ifdef 会被判为无效）
    all_options = base_options + defines + board_flags

    # 添加编译器路径
    compiler_path = 'C:/JL/pi32/bin/clang.exe'

    for source_file in source_files:
        if os.path.exists(source_file):
            command = [compiler_path] + all_options + includes + [source_file]
            compile_commands.append({
                'directory': os.getcwd(),
                'command': ' '.join(command),
                'file': source_file
            })

    # 写入文件
    if compile_commands:
        with open('compile_commands.json', 'w', encoding='utf-8') as f:
            json.dump(compile_commands, f, indent=4)
        print("compile_commands.json generated successfully!")
        return True
    else:
        print("Failed to generate compile_commands.json.")
        return False

def detect_targets():
    """
    自动检测项目中的板级目标
    """
    targets = []

    if os.path.exists('Makefile'):
        with open('Makefile', 'r', encoding='utf-8') as f:
            content = f.read()

        target_pattern = r'^(\w+):$'
        for line in content.split('\n'):
            match = re.match(target_pattern, line.strip())
            if match:
                target = match.group(1)
                if not target.startswith('.') and not target.startswith('clean'):
                    targets.append(target)

    return targets

def repo_root():
    """本脚本所在目录（约定为 SDK 仓库根）。"""
    return os.path.dirname(os.path.abspath(__file__))


# 与原 build_fast.ps1 / clean_build.ps1 一致的默认路径（ac632n / bd19 / spp_and_le）
_DEFAULT_MAKE_SUBDIR = os.path.join('apps', 'spp_and_le', 'board', 'bd19')
_JL_PI32_BIN = r'C:\JL\pi32\bin'

# Cursor/VS Code 任务终端：关闭「终端将被任务重用，按任意键关闭。」
_TASK_PRESENTATION = {
    'echo': True,
    'reveal': 'always',
    'focus': True,
    'panel': 'shared',
    'showReuseMessage': False,
}

_CLEAN_REL_PATHS = [
    'cpu/bd19/tools/sdk.elf',
    'cpu/bd19/tools/sdk.elf.*',
    'cpu/bd19/tools/sdk.lst',
    'cpu/bd19/tools/sdk.map',
    'cpu/bd19/tools/app.bin',
    'cpu/bd19/tools/symbol_tbl.txt',
    'cpu/bd19/tools/text.bin',
    'cpu/bd19/tools/data.bin',
    'cpu/bd19/tools/data_code.bin',
    'cpu/bd19/tools/aec.bin',
    'cpu/bd19/tools/aac.bin',
    'cpu/bd19/tools/aptx.bin',
    'cpu/bd19/tools/bank.bin',
    'cpu/bd19/tools/bank*.bin',
    'cpu/bd19/tools/common.bin',
    'cpu/bd19/tools/download/data_trans/jl_isd.fw',
    'cpu/bd19/tools/download/data_trans/jl_isd.ufw',
    'cpu/bd19/tools/download/data_trans/update.ufw',
    'cpu/bd19/tools/download/data_trans/tone.cfg',
    'cpu/bd19/tools/download/data_trans/cfg_tool.bin',
    'cpu/bd19/tools/download/data_trans/app.bin',
    'cpu/bd19/tools/download/data_trans/bd19loader.bin',
    'cpu/bd19/tools/download/data_trans/p11_code.bin',
    'cpu/bd19/tools/download/data_trans/script.ver',
    os.path.join('apps', 'spp_and_le', 'board', 'bd19', 'objs'),
    'cpu/bd19/sdk_used_list.used',
    'cpu/bd19/sdk.ld',
    'cpu/bd19/tools/isd_config.ini',
]

_BUILD_OUTPUT_CHECK = [
    os.path.join('cpu', 'bd19', 'tools', 'sdk.elf'),
    os.path.join('cpu', 'bd19', 'tools', 'app.bin'),
    os.path.join('cpu', 'bd19', 'tools', 'download', 'data_trans', 'update.ufw'),
]


_CUSTOM_SRC_BLOCK_BEGIN = '# >>> AC63_CUSTOM_SRC_AUTOGEN_BEGIN'
_CUSTOM_SRC_BLOCK_END = '# <<< AC63_CUSTOM_SRC_AUTOGEN_END'


def _ensure_bd19_custom_src_autogen_block(root):
    """
    通过 Python 以幂等方式维护 bd19 Makefile 的自定义源码收集块。
    新增 apps/src 下 .c 后，无需再手动编辑 Makefile。
    """
    mk_path = os.path.join(root, _DEFAULT_MAKE_SUBDIR, 'Makefile')
    if not os.path.isfile(mk_path):
        print(f'未找到 Makefile，跳过自定义源码收集块同步: {mk_path}')
        return True

    try:
        with open(mk_path, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
    except OSError as e:
        print(f'读取 Makefile 失败: {e}')
        return False

    block = (
        f'{_CUSTOM_SRC_BLOCK_BEGIN}\n'
        '# 自动收集 apps/src 下的自定义源码（最多递归 4 层）\n'
        'APP_SRC_ROOT := ../../../../apps/src\n'
        'APP_CUSTOM_SRC := $(foreach d,$(APP_SRC_ROOT) $(wildcard $(APP_SRC_ROOT)/*) $(wildcard $(APP_SRC_ROOT)/*/*) $(wildcard $(APP_SRC_ROOT)/*/*/*),$(wildcard $(d)/*.c))\n'
        '# 需要暂时排除的文件（未完成或实验文件）\n'
        'APP_CUSTOM_SRC_EXCLUDE := \\\n'
        '\t../../../../apps/src/hal/led/led.c\n'
        'c_SRC_FILES += $(filter-out $(APP_CUSTOM_SRC_EXCLUDE),$(APP_CUSTOM_SRC))\n'
        f'{_CUSTOM_SRC_BLOCK_END}\n'
    )

    new_content = content
    if _CUSTOM_SRC_BLOCK_BEGIN in content and _CUSTOM_SRC_BLOCK_END in content:
        pattern = re.compile(
            rf'{re.escape(_CUSTOM_SRC_BLOCK_BEGIN)}.*?{re.escape(_CUSTOM_SRC_BLOCK_END)}\n?',
            re.S,
        )
        new_content = pattern.sub(block, content, count=1)
    else:
        anchor = '# 需要编译的 .S 文件'
        if anchor in content:
            new_content = content.replace(anchor, f'{block}\n{anchor}', 1)
        else:
            if not content.endswith('\n'):
                content += '\n'
            new_content = f'{content}\n{block}'

    if new_content == content:
        return True

    try:
        with open(mk_path, 'w', encoding='utf-8', newline='\n') as f:
            f.write(new_content)
        print(f'已同步自定义源码收集块: {mk_path}')
        return True
    except OSError as e:
        print(f'写入 Makefile 失败: {e}')
        return False


def _build_path_env(root):
    """等同 PS1：把 tools/utils 与 JL pi32/bin 插到 PATH 前面。"""
    tools_utils = os.path.join(root, 'tools', 'utils')
    parts = [tools_utils, _JL_PI32_BIN, os.environ.get('PATH', '')]
    return os.pathsep.join(parts)


def _find_make_executable(root):
    """
    在「与 subprocess 相同的 PATH」里找 make。
    不能只用 shutil.which（默认只看当前进程 PATH，不含 JL/tools），否则会误报找不到。
    """
    path_str = _build_path_env(root)
    names = ('make.exe', 'make') if platform.system() == 'Windows' else ('make', 'gmake')

    for d in path_str.split(os.pathsep):
        if not d or not os.path.isdir(d):
            continue
        for name in names:
            candidate = os.path.join(d, name)
            if os.path.isfile(candidate):
                return candidate

    # 兼容：个别环境 PATH 里目录存在但分隔异常，再直接扫两处固定目录
    for base in (os.path.join(root, 'tools', 'utils'), _JL_PI32_BIN):
        for name in names:
            candidate = os.path.join(base, name)
            if os.path.isfile(candidate):
                return candidate

    return None


def run_build_fast():
    """
    快速并行编译（逻辑自原 build_fast.ps1）：make -C apps/spp_and_le/board/bd19 -jN
    """
    root = repo_root()
    print('=======================================')
    print('AC63 快速编译（Python，MIT，与原 build_fast.ps1 对齐）')
    print('=======================================')

    make_bin = _find_make_executable(root)
    if not make_bin:
        print('未在下列 PATH 前缀中找到 make.exe / make：')
        print(f'  {os.path.join(root, "tools", "utils")}')
        print(f'  {_JL_PI32_BIN}')
        print('请确认杰理工具链已安装，且上述目录中存在 make.exe；或自行安装 GNU Make 并加入 PATH。')
        return False

    cores = os.cpu_count() or 1
    jobs = min(cores, 12)
    print(f'检测到 {cores} 个逻辑 CPU，使用 -j{jobs}')

    make_sub = _DEFAULT_MAKE_SUBDIR.replace('/', os.sep)
    print('')
    print(f'开始快速编译（make -C {make_sub} -j{jobs}）...')
    print('=======================================')
    print(f'执行命令: {make_bin} -C {make_sub} -j{jobs}')

    if not _ensure_bd19_custom_src_autogen_block(root):
        return False

    env = os.environ.copy()
    env['PATH'] = _build_path_env(root)

    started = time.time()
    try:
        proc = subprocess.run(
            [make_bin, '-C', make_sub, f'-j{jobs}'],
            cwd=root,
            env=env,
        )
    except OSError as e:
        print('')
        print(f'编译启动失败: {e}')
        return False

    if proc.returncode != 0:
        print('')
        print('=======================================')
        print(f'编译失败，退出码: {proc.returncode}')
        return False

    elapsed = time.time() - started
    minutes = int(elapsed // 60)
    seconds = int(elapsed % 60)
    print('')
    print('=======================================')
    print('编译成功！')
    print(f'编译耗时: {minutes}分{seconds}秒')

    print('')
    print('生成的文件:')
    for rel in _BUILD_OUTPUT_CHECK:
        p = os.path.join(root, rel)
        if os.path.isfile(p):
            kb = round(os.path.getsize(p) / 1024.0, 2)
            print(f'  OK  {rel}  ({kb} KB)')
        else:
            print(f'  缺失  {rel}')

    print('')
    print('=======================================')
    print('快速编译完成！')
    print('=======================================')
    return True


def _rm_path(path):
    if os.path.isdir(path):
        shutil.rmtree(path, ignore_errors=False)
    elif os.path.isfile(path) or os.path.islink(path):
        os.remove(path)


def run_clean_build():
    """
    清理编译产物（逻辑自原 clean_build.ps1）：按固定相对路径与通配删除。
    """
    root = repo_root()
    print('=======================================')
    print('AC63 清理编译文件（Python，MIT，与原 clean_build.ps1 对齐）')
    print('=======================================')

    deleted = 0
    errors = 0

    print('开始清理编译文件...')
    print('=======================================')

    for rel in _CLEAN_REL_PATHS:
        rel_norm = rel.replace('/', os.sep)
        full = os.path.join(root, rel_norm)
        if any(ch in rel_norm for ch in '*?['):
            matches = glob.glob(full)
            if not matches:
                print(f'  跳过（无匹配）: {rel_norm}')
                continue
            for m in matches:
                try:
                    _rm_path(m)
                    print(f'  已清理: {m}')
                    deleted += 1
                except OSError as e:
                    print(f'  清理失败: {m}  ({e})')
                    errors += 1
            continue

        if os.path.exists(full):
            try:
                _rm_path(full)
                print(f'  已清理: {full}')
                deleted += 1
            except OSError as e:
                print(f'  清理失败: {full}  ({e})')
                errors += 1
        else:
            print(f'  跳过（不存在）: {full}')

    print('')
    print('=======================================')
    print('清理完成！')
    print(f'已清理条目: {deleted}')
    print(f'清理失败: {errors}')
    print('=======================================')
    return errors == 0


def prepend_build_clean_tasks(tasks):
    """把快速编译 / 清理插到 tasks 列表最前，「运行任务」里最先看到。"""
    if platform.system() == 'Windows':
        clean_task = {
            "label": "AC63: 清理编译输出（终端: python ... --clean）",
            "type": "shell",
            "command": "cmd",
            "args": ["/c", "chcp 65001>nul && python clangd_board_switcher.py --clean"],
            "problemMatcher": [],
            "detail": "终端或此处运行均可；删除 bd19 相关产物",
            "presentation": {**_TASK_PRESENTATION},
        }
        build_task = {
            "label": "AC63: 快速编译（终端: python ... --build）",
            "type": "shell",
            "command": "cmd",
            "args": ["/c", "chcp 65001>nul && python clangd_board_switcher.py --build"],
            "problemMatcher": [],
            "detail": "终端或此处运行均可；并行 make apps/spp_and_le/board/bd19",
            "presentation": {**_TASK_PRESENTATION},
        }
    else:
        clean_task = {
            "label": "AC63: 清理编译输出（终端: python ... --clean）",
            "type": "shell",
            "command": "python",
            "args": ["clangd_board_switcher.py", "--clean"],
            "problemMatcher": [],
            "detail": "终端或此处运行均可；删除 bd19 相关产物",
            "presentation": {**_TASK_PRESENTATION},
        }
        build_task = {
            "label": "AC63: 快速编译（终端: python ... --build）",
            "type": "shell",
            "command": "python",
            "args": ["clangd_board_switcher.py", "--build"],
            "problemMatcher": [],
            "detail": "终端或此处运行均可；并行 make apps/spp_and_le/board/bd19",
            "presentation": {**_TASK_PRESENTATION},
        }
    tasks.insert(0, clean_task)
    tasks.insert(0, build_task)


def is_bare_script_invocation():
    """
    判断是否等价于「只写了 python clangd_board_switcher.py」、未带任何参数。
    此时用于：无参数即同步 .vscode / .clangd（不必记 --sync-ide）。
    """
    r = sys.argv[1:]
    if len(r) != 1:
        return False
    token = r[0].replace('\\', '/')
    if token.startswith('-'):
        return False
    return os.path.basename(token) == 'clangd_board_switcher.py'


def _print_terminal_build_clean_hint():
    """醒目提示终端命令（避免被其它输出淹没、与 Cursor「运行任务」列表最前几项一致）。"""
    bar = '=' * 72
    print('')
    print(bar)
    print('  【编译 / 清理】在仓库根终端执行（或 Cursor：终端 → 运行任务 → 选最上面 AC63 两项）')
    print('')
    print('    python clangd_board_switcher.py --build')
    print('    python clangd_board_switcher.py --clean')
    print('')
    print('  全部子命令说明:  python clangd_board_switcher.py -h')
    print(bar)
    print('')


def run_sync_ide():
    """重写 .vscode 与 .clangd（含带弹窗的 tasks），不切换板子、不改 compile_commands。"""
    _print_terminal_build_clean_hint()
    print('（接下来写入 .vscode/tasks.json、settings.json 与 .clangd …）')
    print('')
    generate_vscode_config()
    default_board_cfg = os.path.join(repo_root(), _DEFAULT_MAKE_SUBDIR, 'board_config.h')
    generate_clangd_config(
        extra_add_compile_flags=board_config_define_flags(default_board_cfg),
    )
    print(
        "IDE 已同步。在 Cursor：菜单「终端」→「运行任务…」→ 最上方可见「AC63: 快速编译 / 清理」；选「Clangd: 选择板子…」可弹出 Makefile 列表。"
    )
    _print_terminal_build_clean_hint()


def try_restart_clangd_via_editor_cli(no_restart=False):
    """
    在改写 compile_commands / .vscode / .clangd 或编译、清理完成后，
    尽量向已打开的 Cursor 或 VS Code 窗口发送 clangd.restart（等同命令面板）。
    """
    if no_restart:
        return
    v = os.environ.get('CLANGD_BOARD_SWITCHER_NO_RESTART', '').strip().lower()
    if v in ('1', 'true', 'yes', 'on'):
        return
    root = repo_root()
    for exe in ('cursor', 'code'):
        editor = shutil.which(exe)
        if not editor:
            continue
        try:
            r = subprocess.run(
                [editor, '-r', root, '--command', 'clangd.restart'],
                cwd=root,
                capture_output=True,
                text=True,
                timeout=120,
            )
        except (OSError, subprocess.TimeoutExpired):
            continue
        if r.returncode == 0:
            print('已通过 CLI 请求当前窗口执行 clangd.restart。')
            return
    print(
        '（未能通过 CLI 触发 clangd.restart：未找到 cursor/code，或命令失败；可在命令面板手动执行。）'
    )


def generate_clangd_config(extra_add_compile_flags=None):
    """
    生成 .clangd 配置文件。
    extra_add_compile_flags: 追加到 CompileFlags.Add，例如 board_config.h 解析出的 -DCONFIG_BOARD_*。
    """
    extra_add_compile_flags = extra_add_compile_flags or []
    extra_lines = ''.join(f'\n    - {flag}' for flag in extra_add_compile_flags)
    clangd_content = f"""CompileFlags:
  Add:
    - -w
    - -Wno-unknown-warning-option
    - -Wno-error
    - -Wno-cpp
    - -Wno-pragma-once-outside-header
    - -ferror-limit=0
    - -Wno-implicit-function-declaration
    - -Wno-incompatible-function-pointer-types{extra_lines}
  Remove:
    - -Werror
    - -fallow-pointer-null
    - -fprefer-gnu-section
    - -fms-extensions
Diagnostics:
  Suppress:
    - pp_file_not_found
    - file_not_found
    - drv_unknown_argument
    - implicit-function-declaration
  UnusedIncludes: None
  ClangTidy:
    Remove: [readability-identifier-naming]
  SortDiagnosticOutput: false
"""

    with open('.clangd', 'w', encoding='utf-8') as f:
        f.write(clangd_content)

    print(".clangd configuration file generated successfully!")

def generate_vscode_config():
    """
    生成 VSCode / Cursor 配置：
    - tasks.json 使用 inputs（pickString）：在「运行任务」里选「Clangd: 选择板子…」即弹出 Makefile 目标列表；
    - 选完后执行本脚本 --by-name，自动生成 compile_commands 等；不设为默认构建任务，避免占用 Ctrl+Shift+B。
    """
    vscode_dir = '.vscode'
    if not os.path.exists(vscode_dir):
        os.makedirs(vscode_dir)

    targets = detect_targets()
    tasks = []

    if not targets:
        prepend_build_clean_tasks(tasks)
        tasks.append(
            {
                "label": "Clangd: Makefile 中未检测到目标（请用仓库根作工作区）",
                "type": "shell",
                "command": "python",
                "args": [
                    "-c",
                    "print('No targets parsed from ./Makefile. Open the SDK repo root as the workspace folder, then run: python clangd_board_switcher.py')",
                ],
                "problemMatcher": [],
                "group": {"kind": "build", "isDefault": True},
                "presentation": {**_TASK_PRESENTATION},
            }
        )
        tasks_content = {"version": "2.0.0", "tasks": tasks}
    else:
        prepend_build_clean_tasks(tasks)
        tasks.append(
            {
                "label": "Clangd: 选择板子并生成 compile_commands",
                "type": "shell",
                "command": ("cmd" if platform.system() == 'Windows' else "python"),
                "args": (
                    ["/c", "chcp 65001>nul && python clangd_board_switcher.py --by-name ${input:clangdBoardTarget}"]
                    if platform.system() == 'Windows'
                    else ["clangd_board_switcher.py", "--by-name", "${input:clangdBoardTarget}"]
                ),
                "problemMatcher": [],
                "detail": "菜单：终端 → 运行任务… → 点本项 → 顶部弹出列表（无需参数、不占用 Ctrl+Shift+B）",
                "presentation": {**_TASK_PRESENTATION},
            }
        )
        tasks.append(
            {
                "label": "Clangd: 仅同步 IDE 配置（无参数，等同 python clangd_board_switcher.py）",
                "type": "shell",
                "command": ("cmd" if platform.system() == 'Windows' else "python"),
                "args": (
                    ["/c", "chcp 65001>nul && python clangd_board_switcher.py"]
                    if platform.system() == 'Windows'
                    else ["clangd_board_switcher.py"]
                ),
                "problemMatcher": [],
                "detail": "Makefile 目标变了时跑一次；不写 compile_commands",
                "presentation": {**_TASK_PRESENTATION},
            }
        )
        tasks_content = {
            "version": "2.0.0",
            "tasks": tasks,
            "inputs": [
                {
                    "id": "clangdBoardTarget",
                    "type": "pickString",
                    "description": "选择 Makefile / 板级目标",
                    "options": list(targets),
                }
            ],
        }

    with open(os.path.join(vscode_dir, 'tasks.json'), 'w', encoding='utf-8') as f:
        json.dump(tasks_content, f, indent=4)

    settings_content = {
        "clangd.arguments": [
            "--compile-commands-dir=.",
            "--background-index",
            "--header-insertion=iwyu"
        ],
        "clangd.onConfigChanged": "restart"
    }

    with open(os.path.join(vscode_dir, 'settings.json'), 'w', encoding='utf-8') as f:
        json.dump(settings_content, f, indent=4)

    print("VSCode configuration files generated successfully!")

def main():
    _epilog = """
常用（在 SDK 仓库根目录打开终端执行，MIT 开源）:
  python clangd_board_switcher.py --build       快速并行编译（等同 Cursor 任务「AC63: 快速编译」）
  python clangd_board_switcher.py --clean       清理编译产物（等同任务「AC63: 清理编译输出」）
  python clangd_board_switcher.py               仅同步 .vscode / .clangd / 任务列表
  python clangd_board_switcher.py --sync-ide    同上（显式）
  python clangd_board_switcher.py --by-name X  按 Makefile 目标名切换并生成 compile_commands
  python clangd_board_switcher.py N             按 Makefile 目标序号（从 1 起）切换
  python clangd_board_switcher.py -i            终端里交互选序号

Common (run from repo root, MIT):
  python clangd_board_switcher.py --build
  python clangd_board_switcher.py --clean

完成后会尝试通过 cursor/code CLI 触发 clangd.restart；跳过请设 CLANGD_BOARD_SWITCHER_NO_RESTART=1 或加 --no-restart-clangd。
"""
    _desc = (
        'AC63 SDK 单文件工具（MIT）。\n\n'
        '【一眼可见】终端编译 / 清理（仓库根执行）:\n'
        '  python clangd_board_switcher.py --build\n'
        '  python clangd_board_switcher.py --clean\n\n'
        '【帮助】python clangd_board_switcher.py -h\n\n'
        '另有：Makefile 板级切换、compile_commands、无参同步 Cursor 任务等，见下方参数与 epilog。'
    )
    parser = argparse.ArgumentParser(
        description=_desc,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=_epilog.strip(),
    )
    parser.add_argument(
        '--build',
        action='store_true',
        help='终端 / 任务：并行 make（apps/spp_and_le/board/bd19），开源内置逻辑',
    )
    parser.add_argument(
        '--clean',
        action='store_true',
        help='终端 / 任务：删除 bd19 相关编译产物，开源内置逻辑',
    )
    parser.add_argument(
        '--sync-ide',
        action='store_true',
        help='Only regenerate .vscode and .clangd (same as running this script with no arguments)',
    )
    parser.add_argument(
        '-i',
        '--interactive',
        action='store_true',
        help='Terminal interactive mode (list targets and ask for a number)',
    )
    parser.add_argument(
        '--by-name',
        metavar='NAME',
        dest='by_name',
        help='Makefile target name (used by the Cursor/VS Code task after you pick in the popup)',
    )
    parser.add_argument(
        'target',
        nargs='?',
        type=int,
        help='Target index 1-based (optional)',
    )
    parser.add_argument(
        '--no-restart-clangd',
        action='store_true',
        help='完成后不通过 CLI 请求 clangd.restart（也可设环境变量 CLANGD_BOARD_SWITCHER_NO_RESTART=1）',
    )
    args = parser.parse_args()
    nr = args.no_restart_clangd

    if args.build and args.clean:
        print("Use only one of --build or --clean at a time.")
        sys.exit(2)

    if args.build:
        ok = run_build_fast()
        if ok:
            try_restart_clangd_via_editor_cli(no_restart=nr)
        sys.exit(0 if ok else 1)

    if args.clean:
        ok = run_clean_build()
        if ok:
            try_restart_clangd_via_editor_cli(no_restart=nr)
        sys.exit(0 if ok else 1)

    if args.sync_ide:
        run_sync_ide()
        try_restart_clangd_via_editor_cli(no_restart=nr)
        return

    if (
        is_bare_script_invocation()
        and not args.interactive
        and args.target is None
        and args.by_name is None
    ):
        run_sync_ide()
        try_restart_clangd_via_editor_cli(no_restart=nr)
        return

    print("=================================")
    print("      Clangd Board Switcher")
    print("=================================")

    targets = detect_targets()

    if not targets:
        print("No build targets found in Makefile.")
        _print_terminal_build_clean_hint()
        return

    if args.by_name is not None:
        if args.target is not None:
            print("Use either --by-name or a numeric target, not both.")
            sys.exit(2)
        name = args.by_name
        if name not in targets:
            print(f"Unknown target {name!r}. Known targets: {', '.join(targets)}")
            sys.exit(1)
        if generate_compile_commands(name):
            print("\nSetup completed.")
            try_restart_clangd_via_editor_cli(no_restart=nr)
        else:
            sys.exit(1)
        return

    if args.target is not None:
        index = args.target - 1
        if 0 <= index < len(targets):
            target = targets[index]
            if generate_compile_commands(target):
                print("\nSetup completed.")
                try_restart_clangd_via_editor_cli(no_restart=nr)
        else:
            print("Invalid target number.")
    elif args.interactive:
        print("\nAvailable board targets:")
        for i, target in enumerate(targets, 1):
            print(f"{i}. {target}")

        choice = input("\nEnter target number: ")

        try:
            index = int(choice) - 1
            if 0 <= index < len(targets):
                target = targets[index]
                if generate_compile_commands(target):
                    print("\nSetup completed.")
                    try_restart_clangd_via_editor_cli(no_restart=nr)
            else:
                print("Invalid choice.")
        except ValueError:
            print("Please enter a valid number.")
    else:
        run_sync_ide()
        try_restart_clangd_via_editor_cli(no_restart=nr)


if __name__ == "__main__":
    main()