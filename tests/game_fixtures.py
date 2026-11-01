"""Synthetic map/page archives. No original game data is embedded."""
import struct
from test_engine import fixtures
TAG=0xabcd

def map_files(directory, transform=None):
    walls=[107]*4096
    for y in range(64):
        for x in range(64):
            if x in (0,63) or y in (0,63): walls[y*64+x]=1
    for y in range(1,6): walls[y*64+6]=1
    walls[2*64+6]=90
    objects=[0]*4096;objects[2*64+2]=20
    planes=[walls,objects,[7]*4096]
    if transform: transform(planes)
    graph=bytearray(8+38)
    header=bytearray(38)
    for plane,words in enumerate(planes):
        rlew=[8192];i=0
        while i<len(words):
            n=1
            while i+n<len(words) and words[i+n]==words[i] and n<65535: n+=1
            if n>3 or words[i]==TAG: rlew.extend((TAG,n,words[i]))
            else: rlew.extend([words[i]]*n)
            i+=n
        carmack=bytearray(struct.pack('<H',len(rlew)*2))
        for value in rlew:
            if value>>8 in (0xa7,0xa8): carmack.extend(bytes((0,value>>8,value&255)))
            else: carmack.extend(struct.pack('<H',value))
        struct.pack_into('<I',header,plane*4,len(graph))
        struct.pack_into('<H',header,12+plane*2,len(carmack))
        graph.extend(carmack)
    struct.pack_into('<HH',header,18,64,64)
    header[22:38]=b'Synthetic map\x00\x00\x00'
    graph[8:46]=header
    (directory/'MAPHEAD.WL6').write_bytes(struct.pack('<H',TAG)+struct.pack('<I',8)+struct.pack('<I',0xffffffff)*59)
    (directory/'GAMEMAPS.WL6').write_bytes(graph)
    return planes

def vswap_file(directory, walls=8):
    pages=[bytes((page*17+x*3+y)%256 for x in range(64) for y in range(64)) for page in range(walls)]
    # Signed pixel-pool bias: -18+row(32..63) addresses bytes 14..45.
    sprite=struct.pack('<HHH',31,31,6)+struct.pack('<HHHH',128,(-18)&65535,64,0)+bytes(range(100,132))
    pages.extend([sprite]*436+[b'\x00'])
    offset=6+len(pages)*6
    offsets=[]
    for page in pages:offsets.append(offset);offset+=len(page)
    out=struct.pack('<HHH',len(pages),walls,walls+436)
    out+=b''.join(struct.pack('<I',x) for x in offsets)
    out+=b''.join(struct.pack('<H',len(p)) for p in pages)
    (directory/'VSWAP.WL6').write_bytes(out+b''.join(pages))


def game_files(directory):
    fixtures(directory,ui=True)
    # Extend the synthetic graphics archive with the original status-bar slot.
    head=bytearray((directory/'VGAHEAD.WL6').read_bytes())
    graph=(directory/'VGAGRAPH.WL6').read_bytes()
    from test_engine import encode
    # The balanced dictionary fixture makes chunk byte lengths explicit.
    offsets=[int.from_bytes(head[i:i+3],'little') for i in range(0,450,3)]
    chunks={}
    for index,offset in enumerate(offsets[:-1]):
        if offset==0xffffff:continue
        end=next(x for x in offsets[index+1:] if x!=0xffffff)
        chunks[index]=bytearray(graph[offset:end])
    dims=bytearray(bytes(int(f'{v:08b}'[::-1],2) for v in chunks[0][4:]))
    struct.pack_into('<HH',dims,(86-3)*4,320,40)
    for chunk in (*range(19,23),*range(30,36),*range(43,85),*range(91,135)):
        width,height=(48,24) if 91<=chunk<=94 else (8,16) if chunk<=108 else (24,32)
        if chunk in (95,96,97):width,height=8,16
        if chunk<43: width,height=(48,24)
        elif chunk<85: width,height=(16,16) if chunk not in (43,84) else (96,144)
        if chunk in (44,82,83):width,height=8,16
        struct.pack_into('<HH',dims,(chunk-3)*4,width,height)
        chunks[chunk]=struct.pack('<I',width*height)+encode(bytes([chunk])*(width*height))
    chunks[0]=struct.pack('<I',len(dims))+encode(dims)
    chunks[86]=struct.pack('<I',12800)+encode(bytes([86])*12800)
    out=bytearray();offsets=[0xffffff]*150
    for index,chunk in sorted(chunks.items()):offsets[index]=len(out);out.extend(chunk)
    offsets[149]=len(out)
    (directory/'VGAHEAD.WL6').write_bytes(b''.join(x.to_bytes(3,'little') for x in offsets))
    (directory/'VGAGRAPH.WL6').write_bytes(out)
    map_files(directory);vswap_file(directory)


def original_map_oracle(directory):
    """Independent byte-oriented check of all three owned map planes + wall page 1."""
    head=(directory/'MAPHEAD.WL6').read_bytes()
    graph=(directory/'GAMEMAPS.WL6').read_bytes()
    tag=struct.unpack_from('<H',head)[0]
    at=struct.unpack_from('<I',head,2)[0]
    result=bytearray()
    for plane in range(3):
        start=struct.unpack_from('<I',graph,at+plane*4)[0]
        length=struct.unpack_from('<H',graph,at+12+plane*2)[0]
        chunk=graph[start:start+length]
        expanded=struct.unpack_from('<H',chunk)[0]//2
        words=[];cursor=2
        while len(words)<expanded:
            word=struct.unpack_from('<H',chunk,cursor)[0];cursor+=2
            high=word>>8;count=word&255
            if high not in (0xa7,0xa8):words.append(word);continue
            if not count:words.append(high*256+chunk[cursor]);cursor+=1;continue
            if high==0xa7:source=len(words)-chunk[cursor];cursor+=1
            else:source=struct.unpack_from('<H',chunk,cursor)[0];cursor+=2
            assert 0<=source<len(words)
            for i in range(count):words.append(words[source+i])
        assert len(words)==expanded and words[0]==8192
        plane_words=[];i=1
        while len(plane_words)<4096:
            value=words[i];i+=1
            if value!=tag:plane_words.append(value)
            else:
                count,value=words[i:i+2];i+=2;plane_words.extend([value]*count)
        assert len(plane_words)==4096
        result.extend(struct.pack('<4096H',*plane_words))
    swap=(directory/'VSWAP.WL6').read_bytes()
    count=struct.unpack_from('<H',swap)[0]
    offset=struct.unpack_from('<I',swap,10)[0]
    length=struct.unpack_from('<H',swap,6+count*4+2)[0]
    assert length==4096
    result.extend(swap[offset:offset+length])
    return result


def combat_files(directory):
    game_files(directory)
    def objects(planes):
        for x,y,code in ((5,2,110),(9,5,140),(15,15,144),(17,17,180),
                         (3,6,124),(3,4,24),(3,5,27),(3,3,47),(4,3,49),
                         (4,4,52),(4,5,53),(4,6,54),(4,7,43),(26,1,94)):
            planes[1][y*64+x]=code
    map_files(directory,objects)


def campaign_files(directory):
    game_files(directory)
    def setup(p):
        p[0][30*64+30]=1;p[1][30*64+30]=98
        p[0][2*64+4]=21
        p[0][2*64+2]=p[0][2*64+3]=108 # normal elevator, not secret
        for x,y,code in ((10,10,108),(12,10,144),(14,10,180),
                         (16,10,216),(18,10,234),(20,10,252)):
            p[1][y*64+x]=code
    map_files(directory,setup)
    head=bytearray((directory/'MAPHEAD.WL6').read_bytes())
    for n in range(60): struct.pack_into('<I',head,2+4*n,8)
    (directory/'MAPHEAD.WL6').write_bytes(head)
    vswap_file(directory,64)
