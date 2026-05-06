import replicate

output = replicate.run(
    "sczhou/codeformer:7de2ea26c616d5bf2245ad0d5e24f0ff9a6204578a5c876db53142edd9d2cd56",
    input={
        "image": open("input.jpg", "rb"),  # 本地原始图片
        "upscale": 2,
        "face_upsample": True,
        "background_enhance": True,
        "codeformer_fidelity": "0.2"
    }
)

# 有些模型返回 FileOutput，可直接 read() 保存
with open("my-image.png", "wb") as f:
    f.write(output.read())