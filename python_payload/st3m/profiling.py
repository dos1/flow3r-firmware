import sys_kernel
from st3m import settings


class ftop:
    auto_interval_ms = 5000  # maybe this one too idk~
    _auto_countdown_ms = 0
    _max_name_len = 0
    report = ""

    @staticmethod
    def run(delta_t_ms):
        """
        If enabled: Increments a timer by delta_t_ms milliseconds checks
        if a new report is scheduled and if so saves it in ftop.report.
        Returns True if a new report has been generated, else false.
        """
        if settings.onoff_debug_ftop.value and sys_kernel.usb_console_active():
            ftop._auto_countdown_ms -= delta_t_ms
            if ftop._auto_countdown_ms <= 0:
                ftop._auto_countdown_ms = ftop.auto_interval_ms
                ftop.report = ftop.make_report()
                return True
        return False

    @staticmethod
    def make_report():
        """
        Generates a new report for all tasks and returns it.
        """
        snap = sys_kernel.scheduler_snapshot()
        max_name_len = 0
        for task in snap.tasks:
            max_name_len = max(len(task.name), max_name_len)
        ftop._max_name_len = max_name_len

        snap.tasks.sort(key=lambda x: getattr(x, "name"))
        ftop_str = "\n[task cpu loads]\n"
        header_str = " name" + " " * (ftop._max_name_len - 4)
        header_str += " | "
        header_str += "core load" + " " * 20
        header_str += " | "
        header_str += "prio"
        header_str += " | "
        header_str += "state"
        header_str += " | "
        header_str += "aff "
        ftop_str += header_str + "\n"
        for char in header_str:
            if char == "|":
                ftop_str += "+"
            else:
                ftop_str += "-"
        for task in snap.tasks:
            ftop_str += "\n " + ftop.make_task_report(task)
        # do another run to remove self from cpu load measurement
        _ = sys_kernel.scheduler_snapshot()
        return ftop_str

    @staticmethod
    def make_task_report(task):
        """
        Generates a new report for a given tasks and returns it.
        """
        task_str = ""

        name = task.name[: ftop._max_name_len]
        task_str += name + " " * (ftop._max_name_len - len(name))
        task_str += " | "

        cpu = str(task.cpu_percent)
        task_str += " " * (3 - len(cpu))
        task_str += cpu
        task_str += "%   ["
        hashtags = task.cpu_percent // 5
        if hashtags > 20:
            hashtags = 20
        if hashtags < 0:
            hashtags = 0
        task_str += "#" * hashtags
        task_str += "." * (20 - hashtags)
        task_str += "]"

        task_str += " | "
        prio = str(task.current_priority)
        task_str += " " * (3 - len(prio)) + prio + " "
        task_str += " | "
        if task.state == sys_kernel.RUNNING:
            task_str += " RUN "
        elif task.state == sys_kernel.READY:
            task_str += " RDY "
        elif task.state == sys_kernel.BLOCKED:
            task_str += " BLO "
        elif task.state == sys_kernel.SUSPENDED:
            task_str += " SUS "
        elif task.state == sys_kernel.DELETED:
            task_str += " DEL "
        elif task.state == sys_kernel.INVALID:
            task_str += " INV "
        else:
            task_str += " ??? "
        task_str += " | "
        aff = str(task.core_affinity)
        if aff == "3":
            aff = "1+2"
        else:
            aff = " " + aff
        task_str += aff
        return task_str
