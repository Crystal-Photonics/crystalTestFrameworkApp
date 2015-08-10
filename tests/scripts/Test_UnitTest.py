from __future__ import print_function
import sys
#print(sys.path, file=sys.stderr)
sys.path.append("C:\\Users\\tr\\Desktop\\TestFramework\\build-crystalTestFrameworkApp-Desktop_Qt_5_5_0_MinGW_32bit2-Debug\\tests\\scripts")

import cpgUnitTest
import unittest

class TestStringMethods(unittest.TestCase):

	def test_upper(self):
		print("unittest exec")
		self.assertEqual('foo'.upper(), 'FOO')

	#def test_isupper(self):
	#	self.assertTrue('FOO'.isupper())
	#	self.assertFalse('Foo'.isupper())

	#def test_split(self):
	#	s = 'hello world'
	#	self.assertEqual(s.split(), ['hello', 'world'])
		# check that s.split fails when the separator is not a string
	#	with self.assertRaises(TypeError):
	#		s.split(2)

if __name__ == '__main__':
	print("unittest start")

	#print globals()
	#print "sdfksdflsdfk"
	#unittest.main()
	import sys
	print(sys.path)
	print(sys.version)
	cpgUnitTest.run()
	print("adasd")

