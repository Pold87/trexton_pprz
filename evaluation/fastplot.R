edgeflow.1 <- read.csv('../edgeflow_1.csv')
edgeflow.2 <- read.csv('../edgeflow_2.csv')
edgeflow.3 <- read.csv('../edgeflow_3.csv')
edgeflow.all <- cumsum(read.csv('../edgeflow_diff.csv'))

plot(edgeflow.all)

for (i in 1:length(edgeflow.all[,1])) {
 points(edgeflow.all[1:i,], col='blue', pch=21, bg='blue')
 Sys.sleep(0.01)
 }

filtered <- read.csv('../../particle_filter/src/predictions_filtered_lasso.csv')
preds <- read.csv('../../treXton/predictions.csv')
preds.cross <- read.csv('../../treXton/predictions_cross.csv')
sift <- read.csv('../../datasets/board_test_pos.csv')
sift.cross <- read.csv('../../datasets/board_test_2_pos.csv')[,2:3]


edgeflow.all[,1] <- edgeflow.all[,1] + 1146
edgeflow.all[,2] <- edgeflow.all[,2] + 676


print(edgeflow.all)

plot(preds.cross, col='blue')
points(sift.cross, col='green')
points(edgeflow.all, col='red')

#pdf("edgeflow_vs_sift.pdf")

plot(filtered,
     xlim=range(1:1200),
     ylim=range(1:900),
     pch=21, bg='black')


#points(edgeflow.1, col='green', pch=21, bg='green')

points(sift[,2:3], col='red',pch=21,  bg='red')

#dev.off() 
