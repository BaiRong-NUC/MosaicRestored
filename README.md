# 使用CodeFormer进行马赛克去除

## 论文名称:Towards Robust Blind Face Restoration withCodebook Lookup Transformer

## 论文地址:https://arxiv.org/pdf/2206.11253

## 本地运行

1. 在 `app/serving/.env` 中配置 Replicate token：

    ```env
    REPLICATE_API_TOKEN=你的 token
    ```

2. 编译项目：

    ```bash
    cmake --build build
    ```

    编译结果会生成到 `build/app`，并自动复制 `app/wwwroot`、`app/serving` 和运行脚本 `loop`。

3. 启动服务：

    ```bash
    cd build/app
    ./loop start
    ```

    `loop` 会先停止当前已运行的 Python 修复服务和 C++ 前端/代理服务，再将两个服务后台启动。关闭命令行后服务会继续运行。

    Python 修复服务地址为 `http://127.0.0.1:8091/restore`，C++ 前端/代理服务地址为 `http://127.0.0.1:8090/`。浏览器打开 `http://127.0.0.1:8090/`，上传图片后会走 `前端 -> C++ /api/restore -> Python /restore -> C++ -> 前端`。

4. 查看或停止服务：

    ```bash
    ./loop status
    ./loop stop
    ```

    运行日志和 pid 文件会生成在 `build/app/log`：

    ```text
    log/log.pid
    log/serving.log
    log/mosaic_server.log
    ```

    `log/log.pid` 记录 Python 服务和 C++ 服务两个进程的 pid；`log/serving.log` 保存 Python 服务日志；`log/mosaic_server.log` 保存 C++ 服务日志。

5. Python 环境配置：

    脚本默认优先尝试激活 `picture` conda 环境。如果生产环境的 conda 环境名不同，可以启动时指定：

    ```bash
    MOSAIC_CONDA_ENV=你的环境名 ./loop start
    ```

    也可以直接指定 Python 可执行文件：

    ```bash
    PYTHON=/path/to/python ./loop start
    ```

6. 如果 Python 服务地址需要改，可以在启动 C++ 服务前设置：

    ```bash
    MOSAIC_PYTHON_URL=http://127.0.0.1:8091/restore ./loop start
    ```
