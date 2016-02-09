import pandas as pd
import numpy as np
from numpy import genfromtxt

import matplotlib.pyplot as plt

import cv2

img = pd.read_csv("myimage.csv", sep=',', header=None, nrows=1000)
#img = genfromtxt('myimage.csv', delimiter=',', max_rows=100)

img = img.as_matrix()

print(img[0:100, 0:100])

plt.imshow(img[0:1000, 0:1000], 'Greys_r')
plt.show()
cv2.waitKey(0)
