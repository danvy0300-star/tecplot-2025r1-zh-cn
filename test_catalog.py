import ctypes as c, os, json, sys
from pathlib import Path
root=Path(__file__).parent
bin=Path(sys.argv[1])
cookie=os.add_dll_directory(str(bin))
qt=c.CDLL(str(bin/'Qt5Core.dll'))
def func(name,args,ret=None):
    f=getattr(qt,name);f.argtypes=args;f.restype=ret;return f
ptr=c.c_void_p
ctor=func('??0QTranslator@@QEAA@PEAVQObject@@@Z',[ptr,ptr])
fromutf=func('?fromUtf8_helper@QString@@CA?AV1@PEBDH@Z',[ptr,c.c_char_p,c.c_int],ptr)
strdtor=func('??1QString@@QEAA@XZ',[ptr])
load=func('?load@QTranslator@@QEAA_NAEBVQString@@000@Z',[ptr,ptr,ptr,ptr,ptr],c.c_bool)
trans=func('?translate@QTranslator@@UEBA?AVQString@@PEBD00H@Z',[ptr,ptr,c.c_char_p,c.c_char_p,c.c_char_p,c.c_int],ptr)
transdtor=func('??1QTranslator@@UEAA@XZ',[ptr])
def qs(s):
    q=c.c_void_p();b=s.encode();fromutf(c.byref(q),b,len(b));return q
def decode(q):
    p=q.value
    n=c.c_int.from_address(p+4).value
    off=c.c_int64.from_address(p+16).value
    return c.string_at(p+off,n*2).decode('utf-16-le')
translator=c.create_string_buffer(512)
ctor(translator,None)
path=qs(str(root/'tecplot_zh_CN.qm'));empty=qs('')
assert load(translator,c.byref(path),c.byref(empty),c.byref(empty),c.byref(empty)), 'catalog failed to load'
entries=json.loads((root/'catalog.json').read_text(encoding='utf8'))
for en,zh in entries.items():
    result=c.c_void_p()
    trans(translator,c.byref(result),b'TestContext',en.encode(),None,-1)
    assert decode(result)==zh,(en,decode(result),zh)
    strdtor(c.byref(result))
transdtor(translator);strdtor(c.byref(path));strdtor(c.byref(empty))
print(f'PASS: all {len(entries)} entries loaded and translated by the installed Qt 5.15.2.')
