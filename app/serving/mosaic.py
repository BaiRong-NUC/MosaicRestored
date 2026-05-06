from collections.abc import Iterator, Sequence
import os
from pathlib import Path
import tempfile
from typing import Any, Protocol, cast

from dotenv import load_dotenv
from fastapi import FastAPI, HTTPException, Query, Request, Response, status
import replicate
from starlette.concurrency import run_in_threadpool
import uvicorn


SERVICE_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_IMAGE = PROJECT_ROOT / "test" / "output" / "output.png"
MODEL_VERSION = (
    "sczhou/codeformer:7de2ea26c616d5bf2245ad0d5e24f0ff9a6204578a5c876db53142edd9d2cd56"
)

app = FastAPI(title="Mosaic Restored Service")


class ReplicateFileOutput(Protocol):
    url: str

    def read(self) -> bytes: ...


def first_output(result: Any) -> Any:
    if isinstance(result, Iterator):
        return next(result)
    if isinstance(result, Sequence) and not isinstance(result, (str, bytes, bytearray)):
        return result[0]
    return result


def load_service_env() -> None:
    load_dotenv(SERVICE_DIR / ".env")


def ensure_replicate_token() -> None:
    if not os.environ.get("REPLICATE_API_TOKEN"):
        raise RuntimeError("Missing REPLICATE_API_TOKEN in app/serving/.env")


def generate_restored_image(
    image_bytes: bytes,
    upscale: int,
    face_upsample: bool,
    background_enhance: bool,
    codeformer_fidelity: float,
) -> tuple[bytes, str]:
    ensure_replicate_token()

    with tempfile.NamedTemporaryFile(suffix=".jpg") as image_file:
        image_file.write(image_bytes)
        image_file.flush()
        image_file.seek(0)

        result = replicate.run(
            MODEL_VERSION,
            input={
                "image": image_file,
                "upscale": upscale,
                "face_upsample": face_upsample,
                "background_enhance": background_enhance,
                "codeformer_fidelity": codeformer_fidelity,
            },
        )

    output = cast(ReplicateFileOutput, first_output(result))
    return output.read(), output.url


@app.on_event("startup")
def startup() -> None:
    load_service_env()
    ensure_replicate_token()


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/restore")
async def restore(
    request: Request,
    upscale: int = Query(default=2, ge=1, le=4),
    face_upsample: bool = True,
    background_enhance: bool = True,
    codeformer_fidelity: float = Query(default=0.2, ge=0.0, le=1.0),
    save: bool = False,
) -> Response:
    image_bytes = await request.body()
    if not image_bytes:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Request body must contain image bytes.",
        )

    try:
        restored_bytes, output_url = await run_in_threadpool(
            generate_restored_image,
            image_bytes,
            upscale,
            face_upsample,
            background_enhance,
            codeformer_fidelity,
        )
    except RuntimeError as error:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, detail=str(error)
        ) from error
    except Exception as error:
        raise HTTPException(
            status_code=status.HTTP_502_BAD_GATEWAY,
            detail=f"Replicate request failed: {error}",
        ) from error

    if save:
        OUTPUT_IMAGE.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT_IMAGE.write_bytes(restored_bytes)

    return Response(
        content=restored_bytes,
        media_type="image/png",
        headers={"X-Replicate-Output-Url": output_url},
    )


if __name__ == "__main__":
    load_service_env()
    uvicorn.run(app, host="0.0.0.0", port=8091)
