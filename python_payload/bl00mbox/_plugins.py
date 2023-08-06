# SPDX-License-Identifier: CC0-1.0

import sys_bl00mbox


class _Plugin:
    def __init__(self, index):
        self.index = index
        self.plugin_id = sys_bl00mbox.plugin_index_get_id(self.index)
        self.name = sys_bl00mbox.plugin_index_get_name(self.index)
        self.description = sys_bl00mbox.plugin_index_get_description(self.index)

    def __repr__(self):
        return (
            "[plugin "
            + str(self.plugin_id)
            + "] "
            + self.name
            + ": "
            + self.description
        )


class _Plugins:
    pass


plugins = _Plugins()


def _fill():
    plugins_list = {}
    for i in range(sys_bl00mbox.plugin_registry_num_plugins()):
        plugins_list[sys_bl00mbox.plugin_index_get_name(i).replace(" ", "_")] = i

    for name, value in plugins_list.items():
        setattr(plugins, name, _Plugin(value))


_fill()
