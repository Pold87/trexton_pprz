predictions <- read.csv("../predictions.csv")
sift <- read.csv('../test_data/playing_mat_test_positions.csv')

delta_x <- predictions$x - sift$x
delta_y <- predictions$y - sift$y

sqrt_mse_x <- sqrt(mean(delta_x * delta_x));
sqrt_mse_y <- sqrt(mean(delta_y * delta_y));

cat(sprintf("root MSE x is: %f\n", sqrt_mse_x))
cat(sprintf("root MSE y is: %f\ncd ..", sqrt_mse_y))
