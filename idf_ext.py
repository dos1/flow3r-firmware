import copy
import glob
import os
import os.path
import re
import shutil


def action_extensions(base_actions, project_path=os.getcwd()):
    """
    Implementes -g/--generation and BADGE_GENERATION in idf.py, allowing
    switching between badge generations.
    """

    # Map from canonical name to user-supported names.
    GENERATIONS = {
        'p1': ['proto1'],
        'p3': ['proto3'],
        'p4': ['proto4'],
        'p6': ['proto6'],
        'p6spike': ['proto6-spike'],
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
            for _, names in GENERATIONS.items():
                supported += names
            supported = sorted(supported)
            raise Exception(f'Invalid generation: want one of {", ".join(supported)}')


        # Generate .sdkconfig.defaults.generated that contains BADGE23_*
        # options.
        sdkconfig_defaults_path = os.path.join(project_path, 'sdkconfig.defaults')
        sdkconfig_generated_path = os.path.join(project_path, '.sdkconfig.defaults.generated')
        with open(sdkconfig_generated_path, 'w') as f:
            if name == 'p6spike':
                f.write('CONFIG_BADGE23_HW_GEN_P6=y\n')
                f.write('CONFIG_BADGE23_TOP_BOARD_SPIKES=y\n')
            else:
                f.write(f'CONFIG_BADGE23_HW_GEN_{name.upper()}=y\n')
            with open(sdkconfig_defaults_path) as f2:
                f.write(f2.read())

        global_args.define_cache_entry += [
            'SDKCONFIG_DEFAULTS=' + sdkconfig_generated_path,
        ]
        print(global_args.define_cache_entry)

    # Add global options
    extensions = {
        'global_options': [{
            'names': ['-g', '--generation'],
            'help': 'Specify badge generation to build for (one of: proto1, proto3, proto4, proto6, proto6-spiral). Defaults to proto4.',
            'scope': 'shared',
            'multiple': False,
        }],
        'global_action_callbacks': [generation_callback],
        'actions': {},
    }

    return extensions
