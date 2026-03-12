# Apex-Dumper

基于内核驱动的进程内存转储工具，可将指定进程的模块内存导出为二进制文件，并配合 `pe_unmapper.exe` 将原始转储还原为可分析的 PE 文件。

## 功能

- 以 **管理员权限** 运行，通过驱动读取目标进程内存
- 支持按 **PID** 指定进程，可选按 **模块名** 指定基址（如 `r5apex_dx12.exe`）
- 默认按 PE 头中的 `SizeOfImage` 导出整个模块，也可手动指定大小
- 输出原始内存镜像（默认 `out.bin`），再使用 **pe_unmapper** 还原为 PE

## 环境与依赖

- **Windows x64**
- **Visual Studio 2022**（或带 v143/v145 工具链的 VS）
- 本地需自备（**请勿提交到仓库**）：
  - `driver.h` — 驱动接口头文件
  - `key.txt` — 驱动授权/密钥
  - `MD.lib` — 驱动静态库

## 编译

1. 将 `driver.h`、`key.txt`、`MD.lib` 放入项目根目录（与 `DUMPER.cpp` 同级）。
2. 用 Visual Studio 打开 `dumper.sln`，选择 **Release | x64**，生成解决方案。
3. 输出在 `x64\Release\dumper.exe`。

## 使用方法

### 1. 运行 Dumper（需管理员）

```text
dumper.exe <pid> [output_file] [size_bytes] [module_name]
```

| 参数           | 说明 |
|----------------|------|
| `pid`          | 目标进程 PID（必填） |
| `output_file`  | 输出文件名，默认 `out.bin` |
| `size_bytes`   | 要转储的字节数，默认从 PE 头读取 |
| `module_name`  | 可选，模块名如 `r5apex_dx12.exe`；不填则用主模块 |

示例：

```bat
dumper.exe 12345
dumper.exe 12345 out.bin
dumper.exe 12345 out.bin 0 r5apex_dx12.exe
```

### 2. 使用 pe_unmapper 还原 PE

将 `out.bin` 还原为可分析的 PE 文件：

```bat
pe_unmapper.exe /in out.bin
```

`pe_unmapper.exe` 已随仓库提供，用于处理 dumper 生成的 `out.bin`。

## 仓库说明

- **不包含** `driver.h`、`key.txt`、`MD.lib`，以免泄露驱动与授权信息；本地编译前请自行准备并勿提交。
- **包含** `pe_unmapper.exe`，用于将 `out.bin` 编译/还原为 PE。

## 免责声明

本工具仅供学习与研究使用。使用前请确保符合当地法律法规与目标软件的使用条款，不得用于未授权的内存读取或逆向工程。使用本工具产生的一切责任由使用者自行承担。

## License

见项目仓库与作者声明。
