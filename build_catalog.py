from pathlib import Path
import struct, re, json
root=Path(__file__).parent
base={}
for line in (root/'translations.tsv').read_text(encoding='utf8').splitlines():
    if '\t' in line:
        en, zh=line.split('\t',1)
        base[en]=zh
def norm(s):
    return re.sub(r'\s+',' ',s).strip().replace('&','').rstrip('.: ').lower()
lookup={norm(k):v for k,v in base.items()}
entries=dict(base)
# Preserve menu accelerators and punctuation in exact strings found in installed binaries.
def translate(s):
    core=s.strip()
    key=norm(core)
    if key not in lookup:return None
    zh=lookup[key].rstrip('.: ')
    if '&' in core:
        pos=core.find('&')
        if pos+1<len(core) and core[pos+1].isascii() and core[pos+1].isalpha():
            zh+='(&'+core[pos+1].upper()+')'
    if core.endswith('...'):zh+='...'
    elif core.endswith(':'):zh+='：'
    return zh
for p in root.glob('*.strings.txt'):
    for s in p.read_text(encoding='utf8').splitlines():
        t=translate(s)
        if t:entries[s]=t
for extra in (root/'extracted_ui_strings.json', root/'source_strings.json'):
    if extra.exists():
        for s in json.loads(extra.read_text(encoding='utf8')):
            t=translate(s)
            if t:entries[s]=t
for k,v in list(base.items()):
    for suffix in ('',':','...'):
        entries.setdefault(k+suffix,v+('：' if suffix==':' else suffix))
for k,v in lookup.items():
    entries['__tec_zh_norm__:'+k]=v.rstrip('.: ')

def elf(s):
    h=0
    for b in s:
        h=(h<<4)+b
        g=h&0xf0000000
        if g:h^=g>>24
        h&=~g
    return h or 1
def field(tag,b):return bytes([tag])+struct.pack('>I',len(b))+b
messages=bytearray(); hashes=[]
for en,zh in sorted(entries.items()):
    source=en.encode('utf8'); off=len(messages)
    messages+=field(3,zh.encode('utf-16-be'))+field(6,source)+b'\x01'
    hashes.append((elf(source),off))
magic=bytes.fromhex('3cb86418caef9c95cd211cbf60a1bddd')
qm=magic+field(0xa7,b'zh_CN')+field(0x42,b''.join(struct.pack('>II',*p) for p in sorted(hashes)))+field(0x69,messages)
(root/'tecplot_zh_CN.qm').write_bytes(qm)
(root/'catalog.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding='utf8')
print(f'{len(base)} base terms; {len(entries)} exact translations; {len(qm)} bytes')
