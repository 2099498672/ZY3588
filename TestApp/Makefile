CXX = g++
CXXFLAGS = -Wall -Iinclude -I/usr/include/libnl3 -pthread -pthread -ludev -lbluetooth -lusb-1.0 -lpng -lasound -lm -lfftw3
 
OUT_DIR = out

TARGET = $(OUT_DIR)/TestApp
TEST_TARGET = $(OUT_DIR)/TestAppTest

SRCS = src/main.cpp \
       src/Uart.cpp \
       src/protocol/BufferManager.cpp \
       src/protocol/Crc16.cpp \
       src/protocol/ProtocolParser.cpp \
       src/task/TaskHandler.cpp \
       src/task/TaskQueue.cpp \
       src/util/JsonHelper.cpp \
       src/util/jsoncpp.cpp \
       src/util/Timer.cpp \
       src/util/Log.cpp \
       src/util/ZenityDialog.cpp \
       src/hardware/RkGenericBoard.cpp \
       src/hardware/Gpio.cpp \
       src/hardware/Bluetooth.cpp \
       src/hardware/Storage.cpp \
       src/hardware/Serial.cpp \
       src/hardware/Fan.cpp \
       src/hardware/Rtc.cpp \
       src/hardware/BaseInfo.cpp \
       src/hardware/Wifi.cpp \
       src/hardware/Led.cpp \
       src/hardware/Key.cpp \
       src/hardware/Net.cpp \
       src/hardware/Tf.cpp \
       src/hardware/Adc.cpp \
       src/hardware/TypeC.cpp \
       src/hardware/Audio.cpp \
       src/hardware/Camera.cpp \
       src/hardware/VendorStorage.cpp \
       src/hardware/GetDpLanes.cpp \
       src/project/ZY3588/ZY3588.cpp \
       src/project/BoardFactory.cpp \

TEST_SRCS = $(filter-out src/main.cpp, $(SRCS)) src/main_test.cpp

OBJS = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(SRCS))
TEST_OBJS = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(TEST_SRCS))

all: $(OUT_DIR) $(TARGET)

test : $(OUT_DIR) $(TEST_TARGET)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)/hardware
	mkdir -p $(OUT_DIR)/protocol
	mkdir -p $(OUT_DIR)/task
	mkdir -p $(OUT_DIR)/util
	mkdir -p $(OUT_DIR)/project
	mkdir -p $(OUT_DIR)/project/CM3588S2
	mkdir -p $(OUT_DIR)/project/CM3588V2_CMD3588V2
	mkdir -p $(OUT_DIR)/project/CM3576
	mkdir -p $(OUT_DIR)/project/ZY3588

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) -pthread -ludev -lbluetooth -lusb-1.0 -lpng -lasound -lm -lfftw3

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) -pthread -ludev -lbluetooth -lusb-1.0 -lpng -lasound -lm -lfftw3

$(OUT_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean $(OUT_DIR)