from __future__ import annotations

import json
import os
import urllib.error
import urllib.request

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


class GeminiModel:
    """Gemini native REST API client."""

    def __init__(
        self,
        model_id: str,
        api_key: str | None = None,
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
        self.api_key = api_key or os.environ.get("GEMINI_API_KEY") or os.environ.get("GOOGLE_API_KEY") or ""
        if self.api_key == "":
            raise ValueError("Missing Gemini API key. Set api_key in config or GEMINI_API_KEY in env.")

    @property
    def info(self) -> str:
        return f"{self.model_id}_gemini_temp{self.temperature}"

    def infer(self, prompt: str) -> str:
        url = (
            f"https://generativelanguage.googleapis.com/v1beta/models/{self.model_id}:generateContent"
            f"?key={self.api_key}"
        )
        payload = {
            "system_instruction": {
                "parts": [{"text": self.system_prompt}],
            },
            "contents": [
                {
                    "role": "user",
                    "parts": [{"text": prompt}],
                }
            ],
            "generationConfig": {
                "temperature": self.temperature,
                "maxOutputTokens": self.max_tokens,
            },
        }
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            url,
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=120) as resp:
                body = resp.read().decode("utf-8")
        except urllib.error.HTTPError as e:
            detail = e.read().decode("utf-8", errors="ignore")
            raise RuntimeError(f"Gemini API HTTP {e.code}: {detail}") from e

        obj = json.loads(body)
        candidates = obj.get("candidates", [])
        if len(candidates) == 0:
            return ""
        parts = candidates[0].get("content", {}).get("parts", [])
        text = "".join(p.get("text", "") for p in parts)
        return text


def create_model(cfg: dict):
    provider = cfg.get("provider", "openai_compat")
    if provider == "gemini":
        return GeminiModel(
            model_id=cfg["model_id"],
            api_key=cfg.get("api_key"),
            temperature=float(cfg.get("temperature", 0.0)),
            max_tokens=int(cfg.get("max_tokens", 768)),
            system_prompt=cfg.get("system_prompt"),
        )
    if provider == "openai_compat":
        return APIModel(
            model_id=cfg["model_id"],
            api_key=cfg.get("api_key"),
            api_base=cfg.get("api_base"),
            temperature=float(cfg.get("temperature", 0.0)),
            max_tokens=int(cfg.get("max_tokens", 768)),
            system_prompt=cfg.get("system_prompt"),
        )
    raise ValueError(f"Unsupported provider: {provider}")
