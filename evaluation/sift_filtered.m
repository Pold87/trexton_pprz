clc; clear;

sift_filtered = csvread('/home/pold/Documents/Internship/particle_filter/src/sift_filtered_train_sparse.csv', 1);
sift_filtered_test = csvread('/home/pold/Documents/Internship/particle_filter/src/sift_filtered_test_vel.csv', 1);
sift = csvread('/home/pold/Documents/Internship/datasets/board_train_pos.csv', 1)
sift = sift(:, 2:3)
sift_unfiltered = csvread('/home/pold/Documents/Internship/datasets/board_test_pos.csv', 1)
sift_unfiltered = sift_unfiltered(:, 2:3)

opticalflow = csvread('/home/pold/Documents/Internship/treXton/opticalflow.csv')
opticalflow_diff = csvread('/home/pold/Documents/Internship/treXton/opticalflow_diff.csv')
a = cumsum(opticalflow_diff)

opticalflow_opencv = csvread('/home/pold/Documents/Internship/optFlow_comparison/myflow_pap.csv');
a = cumsum(opticalflow_opencv);

opticalflow_opencv_hrv = csvread('/home/pold/Documents/Internship/optFlow_comparison/myflow.csv');
opticalflow_opencv_hrv(:, 2) = opticalflow_opencv_hrv(:, 2) * 1.5;
opticalflow_opencv_hrv(:, 1) = opticalflow_opencv_hrv(:, 1) * 1.5;
b = cumsum(opticalflow_opencv_hrv);

preds = csvread('/home/pold/Documents/Internship/treXton/predictions.csv')
preds_multi = csvread('/home/pold/Documents/Internship/treXton/predictions_multi.csv')
edgeflow = csvread('/home/pold/Documents/Internship/treXton_pprz/particle_filter_preds.csv')
preds2 = csvread('/home/pold/Documents/Internship/treXton/prediction_rf500.csv')
sift_test_cross_unfiltered = csvread('/home/pold/Documents/Internship/datasets/board_test_2_pos.csv', 1)
sift_test_cross_unfiltered = sift_test_cross_unfiltered(:, 2:3)


sift_cross_filtered = csvread('/home/pold/Documents/Internship/particle_filter/src/sift_filtered_test_2.csv', 1)

preds_filtered = csvread('/home/pold/Documents/Internship/particle_filter/src/predictions_filtered.csv', 1)
preds_lasso_filtered = csvread('/home/pold/Documents/Internship/particle_filter/src/predictions_filtered_lasso.csv', 1)

%outlier = find((sift_unfiltered(:, 1) < 0) | (sift_unfiltered(:, 1) > 700) | (sift_unfiltered(:, 2) < 0) | (sift_unfiltered(:, 2) > 1100))
%sift_unfiltered(outlier, :) = [];
%preds(outlier, :) = [];

% speed = sift(2:580, 2:3) - sift(1:579, 2:3) 
% speed_filtered = sift_filtered(2:580, :) - sift_filtered(1:579, :) 
% 
% diff = sift(:, 2:3) - sift_filtered



a(:, 1) = 1000 - a(:, 1)
a(:, 2) = 300 - a(:, 2)

b(:, 1) = 1000 - b(:, 1)
b(:, 2) = 300 - b(:, 2)

plot_complete(a, b, edgeflow)
% plot_time(a)


