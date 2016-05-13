CXX=gcc

TARGET=WAN_OS_Tool_Serial
# CPPFLAGS += -g -DWANP
# CPPFLAGS += -g -DDEBUG
CPPFLAGS += -g
LDFLAGS += -lpthread
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o, $(SRCS))

$(TARGET):$(OBJS)
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

.depend: $(SRCS)
	cscope -Rbq
	$(CXX) -MM $(SRCS) > $@

sinclude .depend

.PHONY : clean
clean:
	rm -f $(TARGET) *.o .depend cscope.*

