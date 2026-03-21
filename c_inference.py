from __future__ import annotations

import os

from dotenv import load_dotenv
from openai import OpenAI


if os.path.exists(".env"):
    load_dotenv(".env", override=True)


class APIModel:
    """OpenAI-compatible chat completion client (remote API only)."""

    def __init__(
        self,
        model_id: str,
        api_key: str | None = None,
        api_base: str | None = None,
        temperature: float = 0.0,
        max_tokens: int = 768,
        system_prompt: str | None = None,
    ):
        self.model_id = model_id
        self.temperature = temperature
        self.max_tokens = max_tokens
        self.system_prompt = system_prompt or (
            "You are an expert in C systems programming, low-level execution semantics, and debugging."
        )

        key = api_key or os.environ.get("OPENAI_API_KEY") or ""
        base = api_base or os.environ.get("OPENAI_BASE_URL")
        if key == "":
            raise ValueError("Missing API key. Set api_key in config or OPENAI_API_KEY in env.")
        self.client = OpenAI(api_key=key, base_url=base)

    @property
    def info(self) -> str:
        return f"{self.model_id}_api_temp{self.temperature}"

    def infer(self, prompt: str) -> str:
        completion = self.client.chat.completions.create(
            model=self.model_id,
            messages=[
                {"role": "system", "content": self.system_prompt},
                {"role": "user", "content": prompt},
            ],
            temperature=self.temperature,
            max_tokens=self.max_tokens,
        )
        text = completion.choices[0].message.content
        return text or ""
