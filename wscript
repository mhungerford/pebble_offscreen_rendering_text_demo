
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

import os.path

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')

    ctx.env.CFLAGS=['-std=c99',
                    '-mcpu=cortex-m4',
                    '-march=armv7-m',
                    '-mthumb',
                    '-msoft-float',
                    '-mfloat-abi=soft',
                    '-ffunction-sections',
                    '-fdata-sections',
                    '-fmessage-length=0',
                    '-fno-strict-aliasing',
                    '-fomit-frame-pointer',
                    '-ffast-math',
                    '-funroll-loops',
                    '-g',
                    '-O3',
                    '-Wall',
                    '-Wextra',
                   #'-Werror',
                    '-Wl,--gc-sections',
                    '-Wno-unused-parameter',
                    '-Wno-error=unused-function',
                    '-Wno-error=unused-variable' ]


    ctx.env.CFLAGS.append('-Wa,-mimplicit-it=always')
    # We are overriding the gcc-arm toolchain include/time.h with our own
    ctx.env.CFLAGS.append('-D_TIME_H_') # just to check/force our version of time.h

    build_worker = os.path.exists('worker_src')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
        target=app_elf)

        if build_worker:
            worker_elf='{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries, js=ctx.path.ant_glob('src/js/**/*.js'))
