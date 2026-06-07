# LICENSE: imported from wingos
import argparse
import math
from dataclasses import dataclass
from random import randint

TRACE_KIND = "scheduler"
DEFAULT_PID = 10


class CpuTreeNode:
    def __init__(self, cpu_id, children=None, is_leaf=False):
        self.cpu_id = cpu_id
        self.children = children or []
        self.is_leaf = is_leaf

    def are_they_siblings(self, a, b):
        if a in self.children and b in self.children:
            return True

        for child in self.children:
            if a in child.children and b in child.children:
                return True

        for child in self.children:
            if child.are_they_siblings(a, b):
                return True

        return False

    def count(self):
        if self.is_leaf:
            return 1
        return sum(child.count() for child in self.children)


@dataclass
class Task:
    name: str
    base_priority: int
    pid: int = DEFAULT_PID
    nice: int = 0
    running: int = 0
    sleeping: int = 0
    old_cpu: int | None = None
    cpu_affinity: CpuTreeNode | None = None
    total_cycles: int = 0

    def tick(self):
        self.total_cycles += 1

    # Lower values are considered better.
    def depriority(self):
        return self.base_priority + self.running - self.sleeping + self.nice


class MultiCPUScheduler:
    def __init__(self, cpu_tree, total_cycles=20):
        self.cpu_tree = cpu_tree
        self.task_queues = [[] for _ in range(128)]
        self.total_cycles = total_cycles
        self.current_cycle = 0
        self.cpu_runned = {}
        self.next_tid = 0
        self.cpu_groups = self._build_cpu_groups()

    def _build_cpu_groups(self):
        groups = {}
        for group_id, branch in enumerate(self.cpu_tree.children):
            self._assign_group(branch, group_id, groups)
        return groups

    def _assign_group(self, node, group_id, groups):
        if node.is_leaf:
            groups[node.cpu_id] = group_id
            return

        for child in node.children:
            self._assign_group(child, group_id, groups)

    def task_queue_id(self, task):
        return min(max(0, task.depriority() + 64), 127)

    def push_task_first(self, task):
        task.name = f"Task-{self.next_tid}"
        self.next_tid += 1
        qid = self.task_queue_id(task)
        self.task_queues[qid].append(task)

    def add_task(self, task):
        qid = self.task_queue_id(task)
        self.task_queues[qid].append(task)

    def task_count(self):
        return sum(len(queue) for queue in self.task_queues) + len(self.cpu_runned)

    def cpu_count(self):
        return sum(cpu.count() for cpu in self.cpu_tree.children)

    def query_shortest_path_id(self, task_queue, cpu, consider_sibling=False):
        best_i = None
        for i, task in enumerate(task_queue):
            if task.cpu_affinity is None:
                task.cpu_affinity = cpu
                return i
            if task.cpu_affinity == cpu:
                return i
            if consider_sibling and self.cpu_tree.are_they_siblings(
                task.cpu_affinity, cpu
            ):
                best_i = i
        return best_i

    def update_runned_tasks(self):
        for i in range(0, len(self.task_queues) - 1):
            if i == 0:
                self.task_queues[i] = self.task_queues[i] + self.task_queues[i + 1]
            else:
                self.task_queues[i] = self.task_queues[i + 1]
        self.task_queues[len(self.task_queues) - 1] = []

        task_count = self.task_count()
        if task_count == 0:
            self.cpu_runned = {}
            return

        avg_priority = sum(
            task.depriority() for queue in self.task_queues for task in queue
        )

        sleep_boost = math.ceil(avg_priority / task_count)
        for queue in self.task_queues:
            for task in queue:
                task.sleeping += sleep_boost

        for cpu_id, task in self.cpu_runned.items():
            task.running += 1
            task.old_cpu = cpu_id
            self.add_task(task)

        self.cpu_runned = {}

    def fix_affinity(self):
        to_fix = list(self.cpu_runned.keys())

        while to_fix:
            cpu = to_fix.pop()
            task = self.cpu_runned[cpu]
            old_cpu = task.old_cpu

            if old_cpu is not None and cpu != old_cpu:
                stealer_task = self.cpu_runned.get(old_cpu)
                if stealer_task is not None and (
                    stealer_task.cpu_affinity != cpu
                    or task.depriority() < stealer_task.depriority()
                ):
                    stealer_task.cpu_affinity = cpu
                    self.cpu_runned[cpu] = stealer_task

                    task.cpu_affinity = old_cpu
                    self.cpu_runned[old_cpu] = task
                    to_fix.append(old_cpu)

    def run_task_queued(self, cpu, queue_id, queue_offset):
        task = self.task_queues[queue_id].pop(queue_offset)
        task.tick()
        task.cpu_affinity = cpu
        self.cpu_runned[cpu] = task

    def tick_all(self):
        chosen = [i for i in range(self.cpu_count())]

        for queue_id, task_queue in enumerate(self.task_queues):
            retried = []
            while chosen:
                cpu = chosen.pop()
                task_index = self.query_shortest_path_id(task_queue, cpu)
                if task_index is not None:
                    self.run_task_queued(cpu, queue_id, task_index)
                else:
                    retried.append(cpu)

            while task_queue and retried:
                cpu = retried.pop()
                self.run_task_queued(cpu, queue_id, 0)

            if not retried:
                break
            chosen = retried

    def emit_tick(self, tick):
        for cpu_id in sorted(self.cpu_runned):
            task = self.cpu_runned[cpu_id]
            print("@st:sched")
            print(f"#name {task.name}")
            print(f"#kind {TRACE_KIND}")
            print(f"#pid {task.pid}")
            print(f"#tick {tick}")
            print(f"#group {cpu_id}")

    def simulate(self):
        print("@st:begin")
        for tick in range(self.total_cycles):
            self.current_cycle = tick
            self.tick_all()
            self.fix_affinity()
            self.emit_tick(tick)
            self.update_runned_tasks()


def build_cpu_tree():
    leaf00 = CpuTreeNode(cpu_id=0, is_leaf=True)
    leaf01 = CpuTreeNode(cpu_id=1, is_leaf=True)
    leaf10 = CpuTreeNode(cpu_id=2, is_leaf=True)
    leaf11 = CpuTreeNode(cpu_id=3, is_leaf=True)
    root1 = CpuTreeNode(cpu_id=-1, children=[leaf00, leaf01], is_leaf=False)
    root2 = CpuTreeNode(cpu_id=-2, children=[leaf10, leaf11], is_leaf=False)
    root = CpuTreeNode(cpu_id=-3, children=[root1, root2], is_leaf=False)
    return root


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate scheduler trace data in .tail format."
    )
    parser.add_argument(
        "--tasks", type=int, default=2048, help="Number of tasks to simulate"
    )
    parser.add_argument(
        "--cycles", type=int, default=8192, help="Number of scheduler ticks"
    )
    parser.add_argument(
        "--priority",
        type=int,
        default=1,
        help="Max random priority assigned to each task.",
    )
    parser.add_argument(
        "--pid",
        type=int,
        default=DEFAULT_PID,
        help="PID value written to each trace event.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    cpu_tree = build_cpu_tree()
    scheduler = MultiCPUScheduler(cpu_tree, total_cycles=args.cycles)

    tasks = [
        Task(
            name=f"Task-{i}",
            base_priority=randint(1, args.priority),
            pid=args.pid,
        )
        for i in range(args.tasks)
    ]
    for task in tasks:
        scheduler.push_task_first(task)

    scheduler.simulate()


if __name__ == "__main__":
    main()
