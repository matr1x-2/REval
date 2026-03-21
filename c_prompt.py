def _load_template(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def build_direct_prompt(task: str, **kwargs) -> str:
    template = _load_template(f"prompts/c_direct_{task}.txt")
    return template.format(**kwargs)


def build_cot_prompt(task: str, **kwargs) -> str:
    template = _load_template(f"prompts/c_cot_{task}.txt")
    return template.format(**kwargs)
