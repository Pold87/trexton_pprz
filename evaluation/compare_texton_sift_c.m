clc; clear;

remove_outliers = 1

% Read predictions from texton framework
y_test_treXton = csvread('../../treXton/predictions.csv');
y_test_treXton_1 = csvread('../../treXton/predictions1.csv');
y_test_treXton_2 = csvread('../../treXton/predictions2.csv');
y_test_treXton_3 = csvread('../../treXton/predictions3.csv');
y_test_treXton_4 = csvread('../../treXton/predictions4.csv');
y_test_treXton_5 = csvread('../../treXton/predictions4.csv');


% Read SIFT ('ground truth')
y_test_sift = csvread('../../image_recorder/mat_test_square_pos.csv', 1);


% find and remove outliers
if remove_outliers
    outliers_sift = find(y_test_sift(:, 2) < 0 | y_test_sift(:, 2) > 676 | y_test_sift(:, 3) < 0 | y_test_sift(:, 3) > 1024); 
    y_test_sift(outliers_sift, :) = [];
    y_test_treXton(outliers_sift, :) = [];
    y_test_treXton_1(outliers_sift, :) = [];
    y_test_treXton_2(outliers_sift, :) = [];
    y_test_treXton_3(outliers_sift, :) = [];
    y_test_treXton_4(outliers_sift, :) = [];
    y_test_treXton_5(outliers_sift, :) = [];
end

m = (y_test_treXton + y_test_treXton_1 + y_test_treXton_2 + y_test_treXton_3 + y_test_treXton_4 + y_test_treXton_5) / 6;

diff_test_0 = y_test_treXton - y_test_sift(:, 2:3);
diff_test_1 = y_test_treXton_1 - y_test_sift(:, 2:3);
diff_test = m - y_test_sift(:, 2:3);