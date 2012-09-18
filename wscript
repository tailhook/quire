#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from waflib.Build import BuildContext
from waflib import Utils, Options
import os.path
import subprocess

APPNAME='quire'
if os.path.exists('.git'):
    VERSION=subprocess.getoutput('git describe').lstrip('v').replace('-', '_')
else:
    VERSION='0.3.14'

top = '.'
out = 'build'

def options(opt):
    import distutils.sysconfig
    opt.load('compiler_c python')
    opt.add_option('--build-shared', action="store_true", dest="build_shared",
        help="Build shared library instead of static")

def configure(conf):
    conf.load('compiler_c python')
    conf.check_python_version((3,0,0))
    conf.env.BUILD_SHARED = Options.options.build_shared
    conf.find_program('astyle', mandatory=False)


def build(bld):
    bld(
        features     = ['c', 'cstlib'],
        source       = [
            'src/yparser.c',
            'src/metadata.c',
            'src/access.c',
            'src/preprocessing.c',
            'src/genheader.c',
            'src/gensource.c',
            'src/options.c',
            'src/cutil.c',
            'src/error.c',
            'src/emitter.c',
            'src/vars.c',
            'src/eval.c',
            'src/access.c',
            'objpath/objpath.c',
            ],
        target       = 'quire',
        includes     = ['include', 'src', '.'],
        cflags       = ['-std=gnu99', '-Wall'],
        )
    bld(
        features     = ['c', 'cprogram'],
        source       = [
            'src/coyaml.c',
            ],
        target       = 'quire-gen',
        includes     = ['include', 'src', '.'],
        cflags       = ['-std=gnu99', '-Wall'],
        use          = 'quire',
        )
    bld(
        features     = ['c', 'cprogram'],
        source       = [
            'src/ytool.c',
            'objpath/objpath.c',
            ],
        target       = 'quire-tool',
        includes     = ['include', 'src', '.'],
        cflags       = ['-std=gnu99', '-Wall'],
        use          = 'quire',
        )

def build_tests(bld):
    build(bld)
    bld.add_group()
    bld(
        features     = ['c', 'cprogram', 'quire'],
        source       = [
            'test/tinytest.c',
            'test/tinyconfig.yaml',
            ],
        target       = 'tinytest',
        includes     = ['include', 'test', 'src'],
        cflags       = ['-std=c99', '-Wall'],
        use          = ['quire'],
        )
    bld(
        features     = ['c', 'cprogram', 'quire'],
        source       = [
            'test/vartest.c',
            'test/vars.yaml',
            ],
        target       = 'vartest',
        includes     = ['include', 'test'],
        cflags       = ['-std=c99', '-Wall'],
        use          = ['quire'],
        )
    """ Non working tests ATM
    bld(
        features     = ['c', 'cprogram', 'quire'],
        source       = [
            'test/compr.c',
            'test/comprehensive.yaml',
            ],
        target       = 'compr',
        includes     = ['include', 'test'],
        cflags       = ['-std=c99', '-Wall'],
        use          = ['quire'],
        config_name  = 'cfg',
        )
    bld(
        features     = ['c', 'cprogram', 'quire'],
        source       = [
            'test/recursive.c',
            'test/recconfig.yaml',
            ],
        target       = 'recursive',
        includes     = ['include', 'test'],
        cflags       = ['-std=c99', '-Wall'],
        use          = ['quire'],
        config_name  = 'cfg',
        )
    """
    bld.add_group()
    diff = 'diff -u ${SRC[0].abspath()} ${SRC[1]}'
    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} -vv -C -P > ${TGT[0]}',
        source=['tinytest', 'examples/tinyexample.yaml'],
        target='tinyexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/tinyexample.out', 'tinyexample.out'],
        always=True)

    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} -C -P > ${TGT[0]}',
        source=['vartest', 'examples/varexample.yaml'],
        target='varexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/varexample.out', 'varexample.out'],
        always=True)

    """ Non working tests ATM
    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} --config-var clivar=CLI -C -P > ${TGT[0]}',
        source=['compr', 'examples/compexample.yaml'],
        target='compexample.out.ws',
        always=True)
    bld(rule="sed -r 's/\s+$//g' ${SRC[0]} > ${TGT[0]}",
        source='compexample.out.ws',
        target='compexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/compexample.out', 'compexample.out'],
        always=True)

    bld(rule='COMPR_LOGLEVEL=7 ./${SRC[0]} -c ${SRC[1].abspath()} --config-var clivar=CLI -C -PP > ${TGT[0]}',
        source=['compr', 'examples/compexample.yaml'],
        target='compexample.out.ws1',
        always=True)
    bld(rule="sed -r 's/\s+$//g' ${SRC[0]} > ${TGT[0]}",
        source='compexample.out.ws1',
        target='compexample.out1',
        always=True)
    bld(rule=diff,
        source=['examples/compexample_comments.out', 'compexample.out1'],
        always=True)

    bld(rule='./${SRC[0]} -c ${SRC[1].abspath()} -C -P > ${TGT[0]}',
        source=['recursive', 'examples/recexample.yaml'],
        target='recexample.out',
        always=True)
    bld(rule=diff,
        source=['examples/recexample.out', 'recexample.out'],
        always=True)
    bld(rule='COMPR_CFG=${SRC[1].abspath()} ./${SRC[0]} -Dclivar=CLI > ${TGT[0]}',
        source=['compr', 'examples/compexample.yaml'],
        target='compr.out',
        always=True)
    bld(rule=diff,
        source=['examples/compr.out', 'compr.out'],
        always=True)
    """


def ytoolcmd(ctx):
    import yaml, difflib
    bldpath = ctx.bldnode.abspath() + '/'
    error = 0
    for file in ctx.path.ant_glob('test/parser/*.test'):
        data, spec = yaml.safe_load_all(file.read())
        for item in spec:
            if 'command-line' in item:
                status, outp = subprocess.getstatusoutput(
                    bldpath + item['command-line'] + ' -f ' + file.abspath())
                status = os.WEXITSTATUS(status)
                if 'result' in item:
                    if status != item['result']:
                        print(file, 'FAILED:', item['command-line'],
                            ": returned", status, "instead", item['result'])
                        error = 1
                if 'output' in item:
                    if outp != item['output']:
                        print(file, 'FAILED:', item['command-line'],
                            ': output is wrong, diff follows')
                        for line in difflib.ndiff(item['output'].splitlines(),
                                                  outp.splitlines()):
                            print('   ', line.rstrip())
                        error = 1
    return error


class ytooltest(BuildContext):
    cmd = 'ytooltest'
    fun = 'ytoolcmd'


class test(BuildContext):
    cmd = 'test'
    fun = 'build_tests'
    variant = 'test'

def dist(ctx):
    ctx.excl = ['.waf*', '*.tar.bz2', '*.zip', 'build',
        '.git*', '.lock*', '**/*.pyc']
    ctx.algo = 'tar.bz2'

def make_pkgbuild(task):
    import hashlib
    task.outputs[0].write(Utils.subst_vars(task.inputs[0].read(), {
        'VERSION': VERSION,
        'DIST_MD5': hashlib.md5(task.inputs[1].read('rb')).hexdigest(),
        }))

class makepkg(BuildContext):
    cmd = 'archpkg'
    fun = 'archpkg'
    variant = 'archpkg'

def archpkg(bld):
    distfile = APPNAME + '-' + VERSION + '.tar.bz2'
    bld(rule=make_pkgbuild,
        source=['PKGBUILD.tpl', distfile, 'wscript'],
        target='PKGBUILD')
    bld(rule='cp ${SRC} ${TGT}', source=distfile, target='.')
    bld.add_group()
    bld(rule='makepkg -f', source=distfile)
    bld.add_group()
    bld(rule='makepkg -f --source', source=distfile)

def bumpver(ctx):
    ctx.exec_command(r"sed -ri.bak 's/(X-Version[^0-9]*)[0-9.]+/\1"+VERSION+"/'"
        " examples/compr.out examples/compexample.out")


### Example of how to use quire in your own waf files ###
"""
from waflib import TaskGen TaskGen.declare_chain(
        name      = 'quire',
        rule      =
        shell     = False,
        ext_in    = '.yaml',
        ext_out   = ['.h', '.c'],
        reentrant = False,
)
"""


from waflib.Task import Task
class quire(Task):
    run_str = ('${SRC[0].abspath()} --source ${SRC[1].abspath()} '
        '--c-header ${TGT[0].abspath()} '
        '--c-source ${TGT[1].abspath()}')


class astyle(Task):
    run_str = ('${ASTYLE} -Y ${SRC}')


from waflib.TaskGen import extension
@extension('.yaml')
def process_src(self, node):
    tg = self.bld.get_tgen_by_name('quire-gen')
    quire = tg.link_task.outputs[0]
    header = node.change_ext('.h')
    source = node.change_ext('.c')
    self.create_task('quire', [quire, node], [header, source])
    if self.bld.env['ASTYLE']:
        self.create_task('astyle', header)
        self.create_task('astyle', source)

    self.source.append(source)
