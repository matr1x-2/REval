import json

def generate_evaluation_report():
    # 1. 将 DREval_data.jsonl 中的基础数据加载到内存字典中，以 task_id 为键
    data_dict = {}
    with open('DREval_data.jsonl', 'r', encoding='utf-8') as f:
        for line in f:
            obj = json.loads(line)
            data_dict[obj['task_id']] = obj

    # 2. 遍历 DREval_tasks.jsonl，将探针任务与基础代码对齐，并输出到文件
    output_file = "reval_merged_info.txt"
    with open('DREval_tasks.jsonl', 'r', encoding='utf-8') as f, \
         open(output_file, 'w', encoding='utf-8') as out:
        
        for i, line in enumerate(f):
            # 这里暂时只处理前两行数据作为演示。如果想处理全部，可以把下面两行注释掉
            if i >= 2: 
                break
            
            task_obj = json.loads(line)
            task_id = task_obj['task_id']
            
            # 从之前加载的字典中提取对应的代码和测试数据
            base_data = data_dict.get(task_id, {})
            code = base_data.get('code', '[未找到代码]')
            inputs = base_data.get('inputs', [])
            outputs = base_data.get('outputs', [])
            
            # 写入合并后的结构化信息
            out.write(f"========== 评估任务: {task_id} ==========\n")
            out.write("【完整原始代码 (Code)】:\n")
            out.write(code.strip() + "\n\n")
            
            out.write("【测试输入与预期输出 (I/O)】:\n")
            for j in range(min(len(inputs), len(outputs))):
                out.write(f"  测试用例 {j}: 输入 {inputs[j]}  ->  预期输出 {outputs[j]}\n")
            out.write("\n")
            
            out.write("【AI 代码理解追踪任务 (Tracing Tasks)】:\n")
            for sub_task in task_obj.get('tasks', []):
                input_idx = sub_task['input_idx']
                out.write(f"  -> 触发指令: {sub_task.get('output_pred')}\n")
                out.write(f"  -> 要求 AI 推演的状态 (基于测试用例 {input_idx}):\n")
                for trace in sub_task.get('task', []):
                    out.write(f"       当运行到第 {trace['lineno']} 行时，变量 '{trace['var']}' 的值是什么？\n")
                out.write("  " + "-"*40 + "\n")
            out.write("\n" + "="*60 + "\n\n")
    
    print(f"数据整合完毕！结果已输出至当前目录下的 {output_file} 文件。")

if __name__ == "__main__":
    generate_evaluation_report()
