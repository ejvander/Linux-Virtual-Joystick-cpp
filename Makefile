
all: python2

python2: 
	g++ main.cpp console.cpp device.cpp vjoy_python.cpp vjoy.cpp -o vjoy -ludev `python2-config --cflags --libs`

python3: 
	g++ -DPYTHON3 main.cpp console.cpp device.cpp vjoy_python.cpp vjoy.cpp -o vjoy -ludev `python3-config --cflags --libs`

clean: 
	-rm vjoy