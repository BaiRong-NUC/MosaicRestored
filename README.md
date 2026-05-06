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

    编译结果会生成到 `build/app`，并自动复制 `app/wwwroot` 和 `app/serving`。

3. 运行启动脚本：

    ```bash
    cd build/app
    ./start_services.sh
    ```

    脚本会同时启动 Python 修复服务 `http://127.0.0.1:8091/restore` 和 C++ 前端/代理服务 `http://127.0.0.1:8090/`。浏览器打开 `http://127.0.0.1:8090/`，上传图片后会走 `前端 -> C++ /api/restore -> Python /restore -> C++ -> 前端`。

如果 Python 服务地址需要改，可以在启动 C++ 服务前设置：

```bash
export MOSAIC_PYTHON_URL=http://127.0.0.1:8091/restore
```
