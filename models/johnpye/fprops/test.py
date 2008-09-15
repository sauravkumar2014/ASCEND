#!/usr/bin/env python
import subprocess, sys, os.path

try:
	species = sys.argv[1]
except:
	print "Run './test.py speciesname' to run its embedded test suite"
	exit(1)

src = '%s.c'%species
if not os.path.exists(src):
	print "No file named '%s' found in current directory" % src
	exit(1)

s = 'gcc ideal.c helmholtz.c %s.c -DTEST -o %s -lm' % (species,species)

print s
p = subprocess.Popen(s.split(' '))

p.wait()

if not p.returncode:
	s = './%s' % species
	print s
	p = subprocess.Popen(s.split(' '))
	

