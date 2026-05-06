from collections.abc import Iterator, Sequence
import os
from pathlib import Path
from typing import Any, Protocol, cast

import replicate
from dotenv import load_dotenv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
INPUT_IMAGE = PROJECT_ROOT / "test" / "pictures" / "test2.jpg"
OUTPUT_IMAGE = PROJECT_ROOT / "test" / "output" / "output.png"


class ReplicateFileOutput(Protocol):
    url: str

    def read(self) -> bytes: ...


def first_output(result: Any) -> Any:
    if isinstance(result, Iterator):
        return next(result)
    if isinstance(result, Sequence) and not isinstance(result, (str, bytes, bytearray)):
        return result[0]
    return result


# Replicate 官方 SDK 内部会自动读取 REPLICATE_API_TOKEN，并把它放进 HTTP 请求头里
load_dotenv(PROJECT_ROOT / ".env")

if not os.environ.get("REPLICATE_API_TOKEN"):
    raise RuntimeError(
        "Missing REPLICATE_API_TOKEN. Run: "
        "export REPLICATE_API_TOKEN='your_replicate_api_token'"
    )


with open(INPUT_IMAGE, "rb") as image:
    result = replicate.run(
        "sczhou/codeformer:7de2ea26c616d5bf2245ad0d5e24f0ff9a6204578a5c876db53142edd9d2cd56",
        input={
            "image": image,  # 本地原始图片
            "upscale": 2,
            "face_upsample": True,
            "background_enhance": True,
            "codeformer_fidelity": 0.2,
        },
    )

output = cast(ReplicateFileOutput, first_output(result))

# To access the file URL:
print(output.url)
# => "https://replicate.delivery/.../output.png"

# To write the file to disk:
with open(OUTPUT_IMAGE, "wb") as file:
    file.write(output.read())
# => output.png written to disk
