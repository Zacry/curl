INCLUDE = -Ithe3 
		  
LIB = -Llib -lcurl -llibrary_the3_jsoncpp_050
                  
OUTPUT  = test_client.out

SOURCES = main.cpp 
		
OBJECTS=$(SOURCES:.cpp=.o)


		
all:$(OUTPUT)

.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $^ 

.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -o $@ $^

$(OUTPUT): $(OBJECTS)
	$(CXX) $(CFLAGS) -g  -o $@ $^ ${LIB}
	
clean:
	rm -f *.o *.~ *.bak	
	rm -f $(OUTPUT)

