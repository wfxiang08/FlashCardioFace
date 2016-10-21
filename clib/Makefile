objects=handler.o cardio_face1.o AlgUtils.o Sin_Table.o Tracking.o TrackingUtil.o

cardioface.swc : $(objects)
	gcc $(objects) -O3 -Wall -swc -o cardioface.swc 

cardio_face1.o :  algorithm.h AlgUtils.h Sin_Table.h Tracking.h TrackingUtil.h
	gcc -O3 -Wall -c cardio_face.c -o cardio_face1.o
	
handler.o : algorithm.h handler.h AlgUtils.h Tracking.h
	gcc -O3 -Wall -c handler.c   -o handler.o 
AlgUtils.o : algorithm.h Sin_Table.h
	gcc -O3 -Wall -c AlgUtils.c  -o AlgUtils.o 
Sin_Table.o : algorithm.h Sin_Table.h
	gcc -O3 -Wall -c Sin_Table.c -o Sin_Table.o
Tracking.o : algorithm.h Tracking.h
	gcc -O3 -Wall -c Tracking.c  -o Tracking.o 
TrackingUtil.o : TrackingUtil.h
	gcc -O3 -Wall -c TrackingUtil.c -o TrackingUtil.o


.PHONY : clean
clean :
	rm  cardioface.swc $(objects)