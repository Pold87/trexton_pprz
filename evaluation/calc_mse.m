% Read predictions from texton framework
pred = csvread('../predictions.csv', 1);

% Read ground truth data (SIFT)

sift = csvread('../../image_recorder/playing_mat_test_positions.csv', 1);

x_pred = pred(:, 2);
x_actual = sift(:, 2);

y_pred = pred(:, 3);
y_actual = sift(:, 3);

delta_x = x_pred - x_actual;
delta_y = y_pred - y_actual;

mean_x = mean(delta_x);
mean_y = mean(delta_y);

var_x = var(delta_x);
var_y = var(delta_y);

sqrt_mse_x = sqrt(mean(delta_x.^2));
sqrt_mse_y = sqrt(mean(delta_y.^2));

fprintf('Root MSE x is %f\n', sqrt_mse_x); 
fprintf('Root MSE y is %f\n', sqrt_mse_y);
