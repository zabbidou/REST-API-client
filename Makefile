client: helpers.cpp requests.cpp buffer.cpp 

build: 
	g++ -g -o client client.cpp helpers.cpp requests.cpp buffer.cpp 

run: client
	./client

clean:
	rm -f *.o client
