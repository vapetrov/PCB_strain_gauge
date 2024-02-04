"""
Various methods of drawing scrolling plots.
"""

from time import perf_counter

import numpy as np
import serial
import pyqtgraph as pg

import serial.tools.list_ports
s = serial.Serial("COM17", 0, timeout=1)

print(pg.__version__)

pg.setConfigOptions(useOpenGL=True, antialias=True)
#pg.setConfigOptions(antialias=True)

win = pg.GraphicsLayoutWidget(show=True)
win.setWindowTitle('pyqtgraph example: Scrolling Plots')


# 1) Simplest approach -- update data in the array such that plot appears to scroll
#    In these examples, the array size is fixed.
p2 = win.addPlot()
p2.setYRange(-25E3, 25E3, padding=0)
data1 = np.zeros(600)
p2.showGrid(x = True, y = True, alpha = 1.0)
curve2 = p2.plot(data1, pen=pg.mkPen((1.2,250,0), width=4), skipFiniteCheck=True)
ptr1 = 0
def update1():
    global data1, ptr1
    data1[:-1] = data1[1:]  # shift data in the array one sample left
                            # (see also: np.roll)
    #data1[-1] = s.readline().decode("utf-8").split("*")[1]
    try:    
        data1[-1] = int(s.readline().decode("utf-8"))
    except:
        pass

    
    ptr1 += 1
    curve2.setData(data1, skipFiniteCheck=True)
    curve2.setPos(ptr1, 0)
    #print(s.readline().decode("utf-8").split("*")[1])
    

timer = pg.QtCore.QTimer()
timer.timeout.connect(update1)

s.read(s.in_waiting)
timer.start(1)


if __name__ == '__main__':
    pg.exec()