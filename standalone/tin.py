from ctypes import *
import sys

tin = CDLL("tin.so")

class TIN(Structure):
    _fields_ = [
        ("fp", c_char_p),
        ("name", c_char_p),
        ("tt", c_void_p),
        ("nrows", c_short),
        ("ncols", c_short),
        ("nodata", c_short),
        ("numTris", c_uint),
        ("numTiles", c_uint),
        ("numPoints", c_uint),
        ("x", c_double),
        ("y", c_double),
        ("cellsize", c_double),
        ("min", c_short),
        ("max", c_short),
        ("tl", c_uint)]

readTinFile = tin.readTinFile
readTinFile.restype = POINTER(TIN)
handle = readTinFile(sys.argv[1])
#res = pointer(c_int(tin_p))
#print tin_p

print tin.printTin(handle)
