from fastfec import FastFEC

with FastFEC() as fastfec:
    with open('13425.fec', 'rb') as f:
        for line in fastfec.parse(f):
            print(line)
