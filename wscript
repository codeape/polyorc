from waflib.Configure import conf

VERSION = '0.0.1'
BUG_ADDRESS = "some git hub link"

top = '.'

def options(opt):
    opt.load('compiler_c')

@conf
def libckok (ctx, libname, libpath):
    ctx.check(header_name=libpath, features='c cprogram',
              okmsg="{0} is present".format(libname),
              errmsg="{0} is not present".format(libname))

def configure(ctx):
    ctx.load('compiler_c')

    ctx.define('ORC_VERSION', VERSION)
    ctx.define('ORC_BUG_ADDRESS', BUG_ADDRESS)
    ctx.recurse('polyorclib')

    ctx.define('ORC_VERSION', VERSION)
    ctx.define('ORC_BUG_ADDRESS', BUG_ADDRESS)
    ctx.recurse('polyorc')

    ctx.define('ORC_VERSION', VERSION)
    ctx.define('ORC_BUG_ADDRESS', BUG_ADDRESS)
    ctx.recurse('polyorcspider')

def build(ctx):
    ctx.recurse('polyorclib')
    ctx.recurse('polyorc')
    ctx.recurse('polyorcspider')
    ctx.recurse('polyorctest')
