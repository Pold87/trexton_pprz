import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


f = pd.read_csv("mypng.csv")

plt.imshow(f, cmap=plt.get_cmap('gray'))
plt.show()
