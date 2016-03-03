import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression, Lasso, Perceptron, SGDRegressor, LassoLars, Ridge
from sklearn.svm import LinearSVR, NuSVR, SVR
from sklearn import ensemble
from sklearn.mixture import GMM
from sklearn.ensemble import RandomForestRegressor
from sklearn.neighbors import KNeighborsRegressor

remove_outliers = True

X_train = pd.read_csv('../training_data/playing_mat_synthetic_train_histograms.csv', header=None).values
X_test = pd.read_csv('../test_data/playing_mat_synthetic_test_histograms.csv', header=None).values

# Read ground truth data (SIFT - compared to synthetic picture of playing mat)
y_train = pd.read_csv('../training_data/synthetic_playing_mat_train_pos.csv').values
y_test = pd.read_csv('../test_data/synthetic_playing_mat_test_pos.csv').values

# find and remove outliers
if remove_outliers:
    mask_train = (y_train[:, 1] < 0) | (y_train[:, 1] > 676) | (y_train[:, 2] < 0) | (y_train[:, 2] > 1024)
    X_train = X_train[~mask_train, :]
    y_train = y_train[~mask_train, :]
#    X_train[outliers_train, :] = [];
#    y_train[outliers_train, :] = [];
    
    mask_test = (y_test[:, 1] < 0) | (y_test[:, 1] > 676) | (y_test[:, 2] < 0) | (y_test[:, 2] > 1024)
    X_test = X_test[~mask_test, :]
    y_test = y_test[~mask_test, :] 

#    X_test[outliers_test, :] = [];
#    y_test[outliers_test, :] = [];

y_train_x = y_train[:, 1];
y_train_y = y_train[:, 2];
y_test_x = y_test[:, 1];
y_test_y = y_test[:, 2];


clf_x.fit(X_train, y_train_x)
clf_y.fit(X_train, y_train_y)


pred_train_x = clf_x.predict(X_train)
print(pred_train_x)
pred_test_x = clf_x.predict(X_test)
pred_test_y = clf_y.predict(X_test)

diff_train_x = pred_train_x - y_train_x

diff_test_x = pred_test_x - y_test_x
diff_test_y = pred_test_y - y_test_y

RMSE_train = np.sqrt(np.mean(np.square(diff_train_x)))
RMSE_test = np.sqrt(np.mean(np.square(diff_test_x)))
RMSE_test_y = np.sqrt(np.mean(np.square(diff_test_y)))

print(RMSE_train)
print(RMSE_test)
print(RMSE_test_y)

