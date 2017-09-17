# -*- coding: utf-8 -*-


from pyqtgraph.Qt import QtCore, QtGui
import pyqtgraph as pg
import pyqtgraph.opengl as gl
import numpy as np

import os,re,sys

import h5py

# MOVE TRANSITION FILES
if os.path.exists('./axion.m.10000'):
    os.rename('./axion.m.10000','./../axion.m.10000')
if os.path.exists('./axion.m.10001'):
    os.rename('./axion.m.10001','./../axion.m.10001')

fileMeasM = sorted([x for x in [y for y in os.listdir("./")] if re.search("axion.m.[0-9]{5}$", x)])
fileMeasR = sorted([x for x in [y for y in os.listdir("./")] if re.search("axion.r.[0-9]{5}$", x)])

if len(fileMeasR) > 0:
	fileHdf5 = h5py.File(fileMeasR[-1], "r")
else:
	fileHdf5 = h5py.File(fileMeasM[-1], "r")

an_contrastmap = 'energy/density' in fileHdf5

if an_contrastmap:
	print('Contrast found')
	Lx    = fileHdf5["/"].attrs.get("Size")
	Ly    = fileHdf5["/"].attrs.get("Size")
	Lz    = fileHdf5["/"].attrs.get("Depth")
	sizeL = fileHdf5["/"].attrs.get("Physical size")
	z = fileHdf5["/"].attrs.get("z")
	con = fileHdf5['energy']['density'].value.reshape(Ly,Lx,Lz)
	print('Size =  (',Lx,'x',Ly,'x',Lz,') in file ',fileHdf5)

# data = con-1
#
# d2 = np.empty(data.shape + (4,), dtype=np.ubyte)
# positive = np.log(np.clip(data, 0, data.max())**2)
# negative = np.log(np.clip(-(data), 0, -data.min())**2)
#
# d2 = np.empty(data.shape + (4,), dtype=np.ubyte)
# d2[..., 0] = positive * (255./positive.max())
# d2[..., 1] = negative * (255./negative.max())
# d2[..., 2] = d2[...,1]
# d2[..., 3] = d2[..., 0]*0.3 + d2[..., 1]*0.3
# d2[..., 3] = (d2[..., 3].astype(float) / 255.) **2 * 255
#
# d2[:, 0, 0] = [255,0,0,100]
# d2[0, :, 0] = [0,255,0,100]
# d2[0, 0, :] = [0,0,255,100]


print('Max contrast = ', con.max())

# data = con-1
#
# d2 = np.empty(data.shape + (4,), dtype=np.ubyte)
# positive = np.log(np.clip(data, 0, data.max())**2)
# #negative = np.log(np.clip(-(data), 0, -data.min())**2)
#
# d2 = np.empty(data.shape + (4,), dtype=np.ubyte)
# d2[..., 0] = positive * (255./positive.max())
# d2[..., 1] = d2[..., 0]
# d2[..., 2] = d2[..., 0]
# d2[..., 3] = d2[..., 0]*0.3 + d2[..., 1]*0.3
# d2[..., 3] = (d2[..., 3].astype(float) / 255.) **2 * 255
#
# d2[:, 0, 0] = [255,0,0,100]
# d2[0, :, 0] = [0,255,0,100]
# d2[0, 0, :] = [0,0,255,100]

L2 = 1

x = np.linspace(-L2, L2, Lx).reshape(Lx,1,1)
y = np.linspace(-L2, L2, Ly).reshape(1,Ly,1)
z = np.linspace(-L2, L2, Lz).reshape(1,1,Lz)
rh2 = 1-np.clip(np.sqrt(x**2 + y**2 +z**2),0,1)

d2 = np.empty(con.shape + (4,), dtype=np.ubyte)
positive = np.clip(con, 0, 10)**2

d2[..., 0] = positive * (255./positive.max())
d2[..., 1] = d2[..., 0]
d2[..., 2] = d2[..., 0]
d2[..., 3] = d2[..., 0]
d2[..., 3] = (d2[..., 3].astype(float) / 255.) **2 * 255
#d2[..., 3] = rh2 * 255

d2[:, 0, 0] = [255,0,0,100]
d2[0, :, 0] = [0,255,0,100]
d2[0, 0, :] = [0,0,255,100]


from pyqtgraph.Qt import QtCore, QtGui
import pyqtgraph.opengl as gl

app = QtGui.QApplication([])
w = gl.GLViewWidget()
w.opts['distance'] = Lx*2
w.show()
w.setWindowTitle('Logathmic density contrast')


v = gl.GLVolumeItem(d2)
v.translate(-int(Lx/2),-int(Ly/2),-int(Lz/2))
w.addItem(v)

ax = gl.GLAxisItem()
w.addItem(ax)

## Start Qt event loop unless running in interactive mode.
if __name__ == '__main__':
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()