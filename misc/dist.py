#!/usr/bin/env python
from Levenshtein import *
def _T(a, b):
	print(distance(a, b), a, b)
_T("hello","yullo")
_T("he","hello")
_T("hello","helo")
_T("helo","hello")
_T("OOE","OOA")
_T("OOE","OOAE")
_T("OOEA","OOAE")
_T("","abc")
_T("abc","")
_T("hello","h")
_T("x","hello")
_T("","")
