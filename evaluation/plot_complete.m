function [] = plot_time( coords, coords2, coords3 )
%PLOT_TIME Summary of this function goes here
%   Detailed explanation goes here

figure;

scatter(coords(:, 1), coords(:, 2), 'MarkerFaceColor',[1 .0 .0], 'MarkerEdgeColor',[1 .0 .0])
xlim([0 max(1300)])
ylim([0 max(800)])
drawnow
hold on;
scatter(coords2(:, 1), coords2(:, 2), 'MarkerFaceColor',[0 1. .0], 'MarkerEdgeColor',[0 1. .0])
hold on;
scatter(coords3(:, 1), coords3(:, 2), 'MarkerFaceColor',[0 .0 1], 'MarkerEdgeColor',[0 0 1])    

end

