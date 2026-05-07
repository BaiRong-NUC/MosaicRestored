# 使用CodeFormer进行马赛克去除

## 论文名称:Towards Robust Blind Face Restoration withCodebook Lookup Transformer

## 论文地址:https://arxiv.org/pdf/2206.11253

## 本地运行

1. 在 `app/serving/.env` 中配置 Replicate token：

    ```env
    REPLICATE_API_TOKEN=你的 token
    ```

    如果需要在图片处理完成后发送微信通知，也在同一个文件中配置微信参数：

    ```env
    APP_ID=你的微信公众号 AppID
    APP_SECRET=你的微信公众号 AppSecret
    USER_ID=接收通知的微信用户 OpenID
    WECHAT_NOTIFY_ENABLED=true
    WECHAT_RESTORE_DONE_MESSAGE=图片处理完毕，请回到页面查看结果。
    ```

    `ACCESS_TOKEN` 可以不配置，服务会通过 `APP_ID` 和 `APP_SECRET` 自动获取；如果已经有可用 token，也可以直接配置 `ACCESS_TOKEN`。默认通知只发送完成提示，如果想把你自己服务器上的结果图链接也附在消息中，可以增加：

    ```env
    WECHAT_NOTIFY_INCLUDE_URL=true
    ```

    服务会把结果图保存到站点静态目录下，并优先根据当前访问域名生成链接；如果部署在反向代理后且需要强制指定外部访问地址，也可以配置：

    ```env
    MOSAIC_PUBLIC_BASE_URL=https://你的域名
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
