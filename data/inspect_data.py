import json

def inspect_keys():
    try:
        with open('DREval_data.jsonl', 'r', encoding='utf-8') as f:
            first_line = f.readline()
            data = json.loads(first_line)
            
            print("=== DREval_data.jsonl 的真实数据结构 ===")
            print(f"总共发现 {len(data.keys())} 个字段。\n")
            
            for key, value in data.items():
                val_str = str(value)
                # 截断过长的字符串，保持终端输出整洁
                if len(val_str) > 80:
                    val_str = val_str[:80] + " ... [已截断]"
                
                print(f"字段名: '{key}'")
                print(f"内容示例: {val_str}")
                print("-" * 40)
                
    except FileNotFoundError:
        print("找不到 DREval_data.jsonl 文件。")

if __name__ == "__main__":
    inspect_keys()
