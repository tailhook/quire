print("charinfo chars[256] = {");
flags = [[] for i in range(256)]
classes = [None for i in range(256)]
flags[ord('\r')].append('printable')
flags[ord('\n')].append('printable')
flags[ord('\t')].append('printable')
for i in range(0x20, 0x7e + 1):
    flags[i].append('printable')
for i in range(0x80, 0xff+1):
    flags[i].append('printable')
    flags[i].append('plain')
for i in ",[]{}":
    flags[ord(i)].append('flow_indicator')
for i in "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-#;/?:@&=+$_.~*'()":
    flags[ord(i)].append('tag')
for i in "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-#;/?:@&=+$,_.!~*'()[]":
    flags[ord(i)].append('uri')
for i in "!\"#$%&\'()*+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\^_`abcdefghijklmnopqrstuvwxyz|~":
    flags[ord(i)].append('plain_flow')
for i in "!\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~":
    flags[ord(i)].append('plain')
for i in "-?:,[]{}#&*!|>'\"%@`":
    classes[ord(i)] = 'indicator'
classes[ord(' ')] = "whitespace"
classes[ord('\t')] = "whitespace"
classes[ord('\r')] = "whitespace"
classes[ord('\n')] = "whitespace"
for i in '0123456789':
    classes[ord(i)] = 'digit'

for i in range(256):
    print('    {{{0}, {1}}},'.format(
        'CHAR_' + classes[i].upper() if classes[i] else '0',
        '|'.join('CHAR_' + f.upper() for f in flags[i]) if flags[i] else '0'))

print("};");

