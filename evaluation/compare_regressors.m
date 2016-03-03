clc; clear;

remove_outliers = 1

% Read predictions from texton framework
%X_train = csvread('../training_data/playing_mat_synthetic_train_histograms.csv');
%X_test = csvread('../test_data/playing_mat_synthetic_test_histograms.csv');

X_train = csvread('../training_data/playing_mat_synthetic_train_color.csv');
X_test = csvread('../test_data/playing_mat_synthetic_test_color.csv');


% Read ground truth data (SIFT - compared to synthetic picture of playing mat)
y_train = csvread('../training_data/synthetic_playing_mat_train_pos.csv', 1);
y_test = csvread('../test_data/synthetic_playing_mat_test_pos.csv', 1);

% find and remove outliers
if remove_outliers
    outliers_train = find(y_train(:, 2) < 0 | y_train(:, 2) > 676 | y_train(:, 3) < 0 | y_train(:, 3) > 1024); 
    X_train(outliers_train, :) = [];
    y_train(outliers_train, :) = [];
    
    outliers_test = find(y_test(:, 2) < 0 | y_test(:, 2) > 676 | y_test(:, 3) < 0 | y_test(:, 3) > 1024); 
    X_test(outliers_test, :) = [];
    y_test(outliers_test, :) = [];
end

y_train_x = y_train(:, 2);
y_train_y = y_train(:, 3);
y_test_x = y_test(:, 2);
y_test_y = y_test(:, 3);

% Least squares
lambda = 0;
w_x = pinv(X_train' * X_train + lambda * eye(size(X_train, 2))) * X_train' * y_train_x;
w_y = pinv(X_train' * X_train + lambda * eye(size(X_train, 2))) * X_train' * y_train_y;

% Evaluate
%w_x = TreeBagger(10, X_train, y_train_x);
%w_y = TreeBagger(10, X_train, y_train_y);

% On training set
preds_train_x = w_x' * X_train'; 
preds_train_y = w_y' * X_train'; 
%preds_train_x = cell2mat(predict(w_x, X_train)'); 

diff_train_x = preds_train_x' - y_train_x;
diff_train_y = preds_train_y' - y_train_y;
sqrt(mean(diff_train_x .^ 2))

% On test set
preds_test_x = w_x' * X_test';
preds_test_y = w_y' * X_test';
%preds_test_x = predict(w_x, X_test)';
diff_test_x = preds_test_x' - y_test_x;
diff_test_y = preds_test_y' - y_test_y;
sqrt(mean(diff_test_x .^ 2))
sqrt(mean(diff_test_y .^ 2))

%fig = qqplot(diff_train_y);
% saveas(fig,'diff_test_x_qqplot.png')


scatter(diff_test_x, diff_test_y)
%title('Difference between SIFT and treXton prediction on testset')
xlabel('\Delta x')
ylabel('\Delta y')
hold on

x = linspace(min(diff_test_x),max(diff_test_x));
y = linspace(min(diff_test_y), max(diff_test_y));
[X,Y] = meshgrid(x,y);

mu = [mean(diff_test_x), mean(diff_test_y)]
Sigma = cov(diff_test_x, diff_test_y);

F = mvnpdf([X(:) Y(:)], mu, Sigma);
F = reshape(F,length(y),length(x));

%figure
[C,h] = contour(x,y, F)


