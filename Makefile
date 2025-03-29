start:
	g++ main.cpp -o simple -lsfml-graphics -lsfml-window -lsfml-system

simple: simple.cpp
	g++ simple.cpp -o simple -lsfml-graphics -lsfml-window -lsfml-system

simple-O3: simple.cpp
	g++ -O3 simple.cpp -o simple-O3 -lsfml-graphics -lsfml-window -lsfml-system

loop_unfolded: loop_unfolded.cpp
	g++ loop_unfolded.cpp -o loop_unfolded -lsfml-graphics -lsfml-window -lsfml-system

loop_unfolded-O3: loop_unfolded.cpp
	g++ -O3 -mavx2 -mavx loop_unfolded.cpp -o loop_unfolded-O3 -lsfml-graphics -lsfml-window -lsfml-system

intrinsic: intrinsic.cpp
	g++ -mavx2 -mavx intrinsic.cpp -o intrinsic -lsfml-graphics -lsfml-window -lsfml-system

intrinsic-O3: intrinsic.cpp
	g++ -O3 -mavx2 -mavx intrinsic.cpp -o intrinsic-O3 -lsfml-graphics -lsfml-window -lsfml-system

clean:
	rm simple simple-O3 loop_unfolded loop_unfolded-O3 main
