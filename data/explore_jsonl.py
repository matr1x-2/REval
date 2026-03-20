import json

file_path = "DREval_tasks.jsonl"  # 或者 DREval_data.jsonl

with open(file_path, 'r', encoding='utf-8') as f:
    for i, line in enumerate(f):
        if i >= 2:  # 只看前2行
            break
        try:
            data = json.loads(line.strip())
            print(f"--- 第 {i+1} 行数据 ---")
            print("Keys (字段名):", list(data.keys()))  # 打印所有字段名
            print("完整内容:")
            print(json.dumps(data, indent=2, ensure_ascii=False))  # 美化打印
            print("\n")
        except json.JSONDecodeError as e:
            print(f"解析错误: {e}")
