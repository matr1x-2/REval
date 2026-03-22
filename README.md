<<<<<<< HEAD
# C-REval: 面向系统代码理解的评测框架

本仓库在 REval 思路上新增了 C 语言评测管线，用于评估大模型对以下场景的执行语义理解能力：

- 指针与间接访问
- 结构体与内存布局相关表达式
- 函数指针调用
- 宏展开参与的语义
- 并发与系统调用场景（通过你提供的 harness/编译参数）

你负责提供 C 数据，框架负责：

- 自动生成评测点（或使用你手工指定的 probe）
- 编译运行 C 程序
- 通过 gdb 采集行执行轨迹和表达式值
- 调用远程大模型 API 做推理
- 输出四类任务指标（coverage/path/state/output）

## 1. 安装

Python 依赖：

```bash
pip install -r requirements.txt
```

系统依赖（必须）：

- gcc
- gdb

## 2. C 数据格式

默认数据文件路径：

- data/CREval_data.jsonl

每行一个 JSON 对象，最小字段：

```json
{
	"task_id": "CREval/0",
	"entry_point": "sum_array",
	"code": "...C source...",
	"inputs": [
		{
			"harness": "int main(){ ... }"
		}
	]
}
```

推荐可选字段：

- compile_flags: 例如 ["-std=gnu11", "-O0", "-g", "-pthread"]
- inputs[i].output: 该输入对应的期望 stdout（output 任务可直接用）
- inputs[i].probe_exprs: 手工指定状态探针（强烈推荐，用于复杂系统代码）

probe_exprs 示例：

```json
"probe_exprs": [
	{"lineno": 42, "expr": "ptr"},
	{"lineno": 57, "expr": "node->next"},
	{"lineno": 88, "expr": "fn_ptr"}
]
```

参考示例见：

- data/CREval_data.example.jsonl

## 3. 生成任务文件

根据 data/CREval_data.jsonl 生成 data/CREval_tasks.jsonl：

```bash
python c_taskgen.py
```

说明：

- 若输入中提供 probe_exprs，则优先使用手工探针。
- 若未提供，框架会按启发式规则自动抽取语句与表达式。

## 4. API 配置（仅远程 API）

先初始化配置模板：

```bash
python c_evaluation.py init-config -i .c_eval_config.json
```

或直接参考：

- .c_eval_config.example.json

需要填写：

- model_id
- api_key
- api_base（OpenAI 兼容接口地址）

## 5. 运行评测

### coverage

```bash
python c_evaluation.py run -i .c_eval_config.json
```

把配置里的 task 改为以下之一可运行对应任务：

- coverage
- path
- state
- output
- consistency（基于最近一次四项任务日志汇总）

输出默认保存到：

- model_generations_c/{task}@{model_info}/{timestamp}.jsonl

## 6. 各任务定义

- coverage: 某行是否被执行
- path: 某行后下一条执行行号
- state: 指定行处表达式值（由 gdb 捕获）
- output: 程序最终 stdout
- consistency: 四任务一致性加权分数

注意：state 当前定义为“断点命中时（执行该行前）的表达式值”，这一点对 C 复杂语义更稳定。

## 7. 关键文件

- c_dataset.py: C 数据结构约束
- c_taskgen.py: C 任务生成
- c_dynamics.py: 编译/执行/gdb 追踪
- c_inference.py: 远程 API 模型调用
- c_prompt.py: C Prompt 装配
- c_evaluation.py: C 评测入口
- prompts/c_direct_*.txt: 直接回答模板
- prompts/c_cot_*.txt: CoT 模板

## 8. 对系统代码场景的建议

- 指针/结构体/函数指针：尽量手工给 probe_exprs，保证评测点有效。
- 宏展开：把关键宏调用行加入 probe_exprs。
- 并发：compile_flags 添加 -pthread，harness 控制线程调度与 join。
- 系统调用：在 harness 中构造可重放输入，保证评测可复现。

## 9. 已知边界

- gdb 状态比较默认是文本匹配，复杂对象可通过 probe_exprs 选择更稳定表达式。
- path 依赖断点轨迹，建议探针覆盖关键控制流行。
- 如需跨文件大型工程评测，建议先把被评函数及必要依赖裁剪到单任务源文件再评测。
=======
基于 REval 框架改编，用于评估大模型对系统底层代码的理解能力。
>>>>>>> 7b3ca29 (init, fork the code, change readme)
