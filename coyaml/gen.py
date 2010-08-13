
from collections import defaultdict
from . import load

class Field(object):
    def __init__(self, name, type):
        self.name = name
        self.type = type
        self.raw_type = type.__class__

class StructureDef(object):
    ctypes = {
        load.Int: 'int',
        load.UInt: 'uint',
        load.Float: 'float',
        load.String: 'string',
        load.Bool: 'bool',
        load.Array: 'array',
        load.Mapping: 'mapping',
        load.File: 'file',
        load.Dir: 'dir',
        }

    def __init__(self, name, fields=None):
        self.name = name
        if fields is None:
            self.fields = fields
        else:
            self.fields = []

    def add_field(self, f):
        self.fields.append(f)

    def format(self, prefix):
        lines = ['typedef struct %s%s_s {' % (prefix, self.name)]
        for f in self.fields:
            if isinstance(f.type, dict):
                lines.append('    // STRUCT')
            else:
                lines.append('    {0} {1};'.format(self.ctypes[f.raw_type],
                    f.name))
        lines.append('} %s%s_t;' % (prefix, self.name))

class ArrayVal(object):
    def __init__(self, value):
        self.value = list(value)

    def format(self, prefix):
        yield 'static {0} {1}_{2}_vars[{3}] = {{'.format(self.value[0].ctype,
            prefix, self.value[0].__class__.__name__, len(self.value))
        for i, val in enumerate(self.value):
            if val is None:
                yield '    {{ NULL }}, // {0}. sentinel'.format(i)
                continue
            yield '    {0}, // {1}. {2}'.format(
                val.format(prefix), i, '.'.join(val.path))
            if getattr(val, 'start_mark', None):
                yield '    //{0}'.format(val.start_mark)
        yield '};'

class CStruct(object):
    fields = ()
    bitmask = True
    def __init__(self, val):
        self.start_mark = val.start_mark
        self.path = val.path
        for k, typ in self.fields:
            if hasattr(val, k):
                setattr(self, k, typ(getattr(val, k)))

    @classmethod
    def array(cls, value):
        return ArrayVal(value)

    def format(self, prefix):
        if not self.fields:
            return '{}'
        res = []
        bitmask = 0
        for i, (k, v) in enumerate(self.fields):
            if hasattr(self, k):
                bitmask |= 1 << i
                val = getattr(self, k)
                if v == str:
                    val = '"{0}"'.format(repr(val)[1:-1])
                elif v == CStruct:
                    if val:
                        val = '&{0}_{1}_vars[{2}]'.format(prefix,
                            val.__class__.__name__, val.index)
                    else:
                        val = "NULL"
                res.append('{0}: {1}'.format(k, val))
        if self.bitmask:
            res.append('bitmask: {0}'.format(bitmask))
        return '{{{0}}}'.format(', '.join(res))

class CInt(CStruct):
    ctype = 'coyaml_int_t'
    fields = (
        ('min', int),
        ('max', int),
        )

class CUInt(CInt):
    ctype = 'coyaml_uint_t'

class CFloat(CStruct):
    ctype = 'coyaml_float_t'
    fields = (
        ('min', float),
        ('max', float),
        )

class CString(CStruct):
    ctype = 'coyaml_string_t'

class CBool(CStruct):
    ctype = 'coyaml_bool_t'

class CArray(CStruct):
    ctype = 'coyaml_array_t'

class CMapping(CStruct):
    ctype = 'coyaml_mapping_t'

class CFile(CStruct):
    ctype = 'coyaml_file_t'
    fields = (
        ('check_existence', bool),
        ('check_dir', bool),
        ('check_writable', bool),
        )

class CDir(CStruct):
    ctype = 'coyaml_dir_t'
    fields = (
        ('check_existence', bool),
        ('check_dir', bool),
        ('check_writable', bool),
        )

class CTransition(CStruct):
    ctype = 'coyaml_transition_t'
    bitmask = False
    fields = (
        ('symbol', str),
        ('callback', None),
        ('prop', CStruct),
        )
    def __init__(self, key, typ, *, path):
        self.symbol = key
        self.prop = typ
        self.path = path
        self.start_mark = typ.start_mark

    @property
    def callback(self):
        return '(coyaml_state_fun)&coyaml_' + self.prop.__class__.__name__

class CTag(CStruct):
    ctype = 'coyaml_tag_t'
    bitmask = False
    fields = (
        ('tagname', str),
        ('tagvalue', int),
        )
    def __init__(self, name, value):
        self.name = name
        self.value = value

class CGroup(CStruct):
    ctype = 'coyaml_group_t'
    fields = (
        ('transitions', CStruct),
        )
    def __init__(self, dic, first_tran, *, path):
        self.path = path
        self.start_mark = dic.start_mark
        self.first_tran = first_tran

    @property
    def transitions(self):
        return self.first_tran

class CCustom(CStruct):
    ctype = 'coyaml_custom_t'
    fields = (
        ('transitions', CStruct),
        )

    @property
    def transitions(self):
        return self.first_tran

class CUsertype(CStruct):
    ctype = 'coyaml_usertype_t'
    fields = (
        ('transitions', CStruct),
        )

    def __init__(self, data, child, *, path):
        self.data = data
        self.child = child
        self.path = path

ctypes = {
    'Int': CInt,
    'Uint': CUInt,
    'Float': CFloat,
    'String': CString,
    'Bool': CBool,
    'Array': CArray,
    'Mapping': CMapping,
    'File': CFile,
    'Dir': CDir,
    'Struct': CCustom,
    'Usertype': CUsertype,
    'dict': CGroup,
    'transition': CTransition,
    }

class GenCCode(object):

    def __init__(self, cfg):
        self.cfg = cfg
        self.lines = [
            '/* THIS IS AUTOGENERATED FILE */',
            '/* DO NOT EDIT!!! */',
            '#include <coyaml_src.h>',
            '',
            'static coyaml_transition_t '+self.cfg.name+'_CTransition_vars[];',
            ]
        self.types = defaultdict(list)
        self.type_visitor(cfg.data)
        self.types['dict'].reverse() # Root item must be first
        for k, t in cfg.types.items():
            self.type_visitor(t, ('__types__', k))
        self.type_enum()
        self.make_types()

    def make_types(self):
        types = self.types.copy()
        tran = types.pop('transition', [])
        for tname, items in types.items():
            self.lines.append('')
            self.lines.extend(ctypes[tname].array(items)
                .format(prefix=self.cfg.name))
        self.lines.append('')
        self.lines.extend(CTransition.array(tran)
            .format(prefix=self.cfg.name))

    def type_visitor(self, data, path=()):
        data.path = path
        if isinstance(data, load.Usertype):
            mem = load.Group(data.members)
            mem.start_mark = data.start_mark
            child = self.type_visitor(mem, path=path)
            res = CUsertype(data, child, path=path)
            self.types['Usertype'].append(res)
        elif isinstance(data, dict):
            children = []
            for k, v in data.items():
                chpath = path + (k,)
                typ = self.type_visitor(v, chpath)
                children.append(CTransition(k, typ, path=chpath))
            if children:
                children.append(None)
                self.types['transition'].extend(children)
            res = CGroup(data, children and children[0] or None, path=path)
            self.types['dict'].append(res)
        else:
            tname = data.__class__.__name__
            res = ctypes[tname](data)
            self.types[tname].append(res)
        return res

    def type_enum(self):
        for lst in self.types.values():
            for i, t in enumerate(lst):
                if t is None:
                    continue
                t.index = i

    def print(self):
        for line in self.lines:
            print(line)

def main():
    from .cli import simple
    from .load import load
    cfg, inp, opt = simple()
    with inp:
        load(inp, cfg)
    GenCCode(cfg).print()

if __name__ == '__main__':
    from .gen import main
    main()
