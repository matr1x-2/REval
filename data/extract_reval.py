import json

def extract_evaluation_data():
    # 1. 读取包含原代码和答案的基础数据文件
    data_dict = {}
    try:
        with open('DREval_data.jsonl', 'r', encoding='utf-8') as f:
            for line in f:
                obj = json.loads(line)
                data_dict[obj['task_id']] = obj
    except FileNotFoundError:
        print("未找到 DREval_data.jsonl，请确保它在当前目录下。")
        return

    # 2. 读取任务追踪点文件，并与基础数据合并展示
    with open('DREval_tasks.jsonl', 'r', encoding='utf-8') as f:
        for i, line in enumerate(f):
            if i >= 2:  # 这里只打印前 2 个任务作为调试
                break
                
            task_obj = json.loads(line)
            task_id = task_obj['task_id']
            
            # 从 DREval_data 提取对应的源代码信息
            base_data = data_dict.get(task_id, {})
            
            print(f"========== 评估任务: {task_id} ==========")
            # 这里的 'prompt' 和 'canonical_solution' 是大多数代码数据集的通用字段名
            # 如果报错，说明 DREval_data 里的键名不同，你可以 print(base_data.keys()) 查看一下
            prompt = base_data.get('prompt', '[未找到代码上下文]')
            solution = base_data.get('canonical_solution', '[未找到参考实现]')
            
            print("【完整待评估代码】:")
            print(prompt + solution)
            print("-" * 40)
            
            print("【AI 需要推演的执行轨迹与测试】:")
            for sub_task in task_obj['tasks']:
                print(f"  -> 测试调用: {sub_task['output_pred']}")
                print(f"  -> 考察 AI 对以下状态的理解:")
                for trace in sub_task['task']:
                    print(f"       第 {trace['lineno']} 行 | 变量名: {trace['var']}")
                print("  " + "." * 38)
            print("\n")

if __name__ == "__main__":
    extract_evaluation_data()
