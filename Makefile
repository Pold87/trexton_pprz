all: trexton_classification trexton_regression

trexton_classification: trexton.c
	gcc -O2 -Wextra -ansi -g -pthread -I. -I/home/pold/paparazzi/sw/airborne/modules/computer_vision/lib -I/home/pold/paparazzi/sw/airborne/arch/linux -I/home/pold/paparazzi/sw/include -I/home/pold/paparazzi/sw/airborne/modules/computer_vision/lib/v4l -o trexton_pprz trexton.c lib/v4l/v4l2.c lib/vision/image.c udp_socket.c lib/encoding/jpeg.c lib/encoding/rtp.c readcsv.c texton_helpers.c texton_helpers.h libcsv.c -lm -std=c99 -Wno-implicit-function-declaration

trexton_regression:
	gcc -O2 -ansi -g -pthread -I. -I/home/pold/paparazzi/sw/airborne/modules/computer_vision/lib -I/home/pold/paparazzi/sw/airborne/arch/linux -I/home/pold/paparazzi/sw/include -I/home/pold/paparazzi/sw/airborne/modules/computer_vision/lib/v4l -o trexton_regression trexton_regression.c lib/v4l/v4l2.c lib/vision/image.c udp_socket.c lib/encoding/jpeg.c lib/encoding/rtp.c readcsv.c texton_helpers.c texton_helpers.h libcsv.c -lm -std=c99 -Wno-implicit-function-declaration

run:
	./trexton_regression

clean:
	rm trexton_pprz trexton_regression
