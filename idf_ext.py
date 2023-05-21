import copy
import glob
import os
import os.path
import re
import shutil


def action_extensions(base_actions, project_path=os.getcwd()):
    """
    Implementes -g/--generation and BADGE_GENERATION in idf.py, allowing
    switching between badge generations and sdkconfig default files.
    """

    # Map from canonical name to user-supported names.
    GENERATIONS = {
        'p1': ['proto1'],
        'p3': ['proto3'],
        'p4': ['proto4'],
        'p5': ['adi-less'],
    }

    def generation_callback(ctx, global_args, tasks):
        """
        Implements translation from set -g/--generation and BADGE_GENERATION
        into CMake cache entries.
        """
        generation = global_args.generation
        if generation is None:
            generation = os.environ.get('BADGE_GENERATION', 'proto4')

        name = None
        if generation in GENERATIONS:
            name = generation
        else:
            for gen, names in GENERATIONS.items():
                if generation in names:
                    name = gen
                    break
        if name is None:
            supported = []
            supported += GENERATIONS.keys()
            for _, names in GENERATIONS.values():
                supported += names
            sort(supported)
            raise Exception(f'Invalid generation: want one of {", ".join(supported)}')

        sdkconfig_name = 'sdkconfig.' + name
        sdkconfig_path = os.path.join(project_path, sdkconfig_name)
        if not os.path.exists(sdkconfig_path):
            raise Exception(f'Missing sdkconfig file {sdkconfig_name}')
        cache_entries = {
            'SDKCONFIG_DEFAULTS': sdkconfig_path,
        }
        print(cache_entries)
        global_args.define_cache_entry = list(global_args.define_cache_entry)
        global_args.define_cache_entry.extend(['%s=%s' % (k, v) for k, v in cache_entries.items()])

    # Add global options
    extensions = {
        'global_options': [{
            'names': ['-g', '--generation'],
            'help': 'Specify badge generation to build for (one of: proto1, proto3, proto4, adiless). Defaults to proto4.',
            'scope': 'shared',
            'multiple': False,
        }],
        'global_action_callbacks': [generation_callback],
        'actions': {},
    }

    return extensions
