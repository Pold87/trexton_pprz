trexton_pprz: trexton.c
	gcc -O2 -Wall -Wextra -ansi -g -pthread -I. -I/home/pold/paparazzi/sw/airborne/modules/computer_vision/lib -I/home/pold/paparazzi/sw/airborne/arch/linux -I/home/pold/paparazzi/sw/include -I/home/pold/paparazzi/sw/airborne/modules/computer_vision/lib/v4l -o trexton_pprz trexton.c lib/v4l/v4l2.c lib/vision/image.c udp_socket.c lib/encoding/jpeg.c lib/encoding/rtp.c readcsv.c libcsv.c -lm -std=c99 -Wno-implicit-function-declaration

run:
	./trexton_pprz

clean:
	rm *.o trexton_pprz
