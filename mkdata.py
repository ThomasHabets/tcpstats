#!/usr/bin/python

from listener import DATA

def main():
    print "enum DATA {"
    for num,name in enumerate(DATA):
        print "%8sDATA_%s = %d," % ("", name.upper(), num)
    print "};"

if __name__ == '__main__':
    main()
