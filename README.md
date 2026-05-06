# 使用Self-Supervised-photo进行马赛克去除

## 本地运行

1. 在 `app/serving/.env` 中配置 Replicate token：

    ```env
    REPLICATE_API_TOKEN=你的 token
    ```

2. 启动 Python 修复服务：

    ```bash
    python app/serving/mosaic.py
    ```

    默认监听 `http://127.0.0.1:8091/restore`。

3. 编译并启动 C++ 前端/代理服务：

    ```bash
    cmake --build build
    cd build/bin
    ./mosaic_server
    ```

    浏览器打开 `http://127.0.0.1:8090/`，上传图片后会走 `前端 -> C++ /api/restore -> Python /restore -> C++ -> 前端`。

如果 Python 服务地址需要改，可以在启动 C++ 服务前设置：

```bash
export MOSAIC_PYTHON_URL=http://127.0.0.1:8091/restore
```
