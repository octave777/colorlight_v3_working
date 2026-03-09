CXX = g++
CXXFLAGS = -Wall -O2 `pkg-config --cflags opencv4 freetype2`
LIBS = `pkg-config --libs opencv4 freetype2`

TARGET = colorlight_app
OBJS = main.o ColorLightController.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
