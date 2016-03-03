clear, clc;
% Sensor model if map is divided into areas

sift = csvread('/home/pold/Documents/Internship/datasets/board_test_pos.csv', 1)
sift= sift(1:190, 2:3)

preds = csvread('/home/pold/Documents/Internship/particle_filter/src/predictions_filtered_lasso.csv', 1);

num_areas_w = 3;
num_areas_h = 3;

image_file = '/home/pold/Documents/Internship/draug/img/sparse_board.jpg'; 
img = imread(image_file);
[img_height, img_width, z1] = size(img);

w_step = img_width / num_areas_w;
h_step = img_height / num_areas_h;

%areas_sift = zeros([num_areas_w, num_areas_h, length(sift)]);
%preds_for_area = zeros([num_areas_w, num_areas_h, length(sift)]);
%diff_per_area = zeros([num_areas_w, num_areas_h, length(sift)]);

sift_area_label = zeros(length(sift), 1);
preds_area_label = zeros(length(preds), 1);


img(int16(h_step):int16(h_step):end,:,:) = 0;       %# Change every tenth row to black
img(:, int16(w_step):int16(w_step):end,:) = 0;       %# Change every tenth column to black
%imshow(img);                  %# Display the image


% Extract predictions per region
for i = 1:num_areas_w
    for j = 1:num_areas_h
        mask_sift = sift(:, 1) > w_step * (i - 1) & ...
               sift(:, 1) < w_step * i & ...
               sift(:, 2) > h_step * (j - 1) &...
               sift(:, 2) < h_step * j;
           
        mask_preds = preds(:, 1) > w_step * (i - 1) & ...
               preds(:, 1) < w_step * i & ...
               preds(:, 2) > h_step * (j - 1) &...
               preds(:, 2) < h_step * j;           
           
        %areas_sift(i, j) = sift(mask);
        %preds_for_area(i, j) = preds(mask);
        
        sift_area_label(find(mask_sift))= (i - 1) * num_areas_h + j;
        preds_area_label(find(mask_preds)) = (i - 1) * num_areas_h + j;
        
    end
end

cm = confusionmat(sift_area_label, preds_area_label, 'order', 0:(num_areas_h * num_areas_w))
%cm_self = confusionmat(sift_area_label, sift_area_label)

for pos = 1:9
    
    diff = sift(find(preds_area_label == pos), :) - preds(find(preds_area_label == pos), :);
    fprintf('Tile num: %d\n', pos);
    fprintf('Length %d\n', length(diff));
    fprintf('Standard deviation:\n');
    disp(std(diff))
    
end

%imagesc(cm);
%colorbar;