clc; clear;

sift_filtered = csvread('/home/pold/Documents/Internship/particle_filter/src/sift_filtered_train_sparse.csv', 1);
sift_filtered_test = csvread('/home/pold/Documents/Internship/particle_filter/src/sift_filtered_test_vel.csv', 1);
sift = csvread('/home/pold/Documents/Internship/datasets/board_train_pos.csv', 1)
sift = sift(:, 2:3)
sift_unfiltered = csvread('/home/pold/Documents/Internship/datasets/board_test_pos.csv', 1)
sift_unfiltered = sift_unfiltered(:, 2:3)

opticalflow = csvread('/home/pold/Documents/Internship/treXton/opticalflow.csv')
preds = csvread('/home/pold/Documents/Internship/treXton/predictions.csv')
preds_multi = csvread('/home/pold/Documents/Internship/treXton/predictions_multi.csv')
edgeflow = csvread('/home/pold/Downloads/particle_filter_preds.csv')
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

plot_complete(preds_lasso_filtered, edgeflow, opticalflow)
% plot_time(edgeflow)


